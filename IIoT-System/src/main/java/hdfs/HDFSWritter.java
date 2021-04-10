package hdfs;

import flink.Main;
import lombok.Getter;
import org.apache.commons.io.IOUtils;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.*;
import org.apache.log4j.Logger;
import org.influxdb.dto.Query;
import org.influxdb.dto.QueryResult;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.net.URI;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import static flink.Main.*;
import static influxdb.ColorDataSink.DATA_FIELD;
import static influxdb.ColorDataSink.QUALITY_FIELD;

@Getter
public class HDFSWritter implements Runnable {
    private static final int maxNumbColorDataSetsInDB = 50;
    private static final int checkSeriesIntervalSec = 30;
    private static final String HADOOP_USER_NAME = "root";

    private String hdfsUri;
    private FileSystem fs;
    private String colorDataDir;
    private String colorDataFile;

    private transient volatile boolean running;
    private static Logger logger = Logger.getLogger(HDFSWritter.class);

    public HDFSWritter(String hdfsUri, String colorDataDir, String colorDataFile) {
        this.colorDataFile = colorDataFile;
        this.colorDataDir = colorDataDir;
        this.hdfsUri = hdfsUri;
        try {
            connect();
            running = true;
        } catch (IOException e) {
            running = false;
        }
    }

    public void connect() throws IOException {
        Configuration conf = new Configuration();
        System.setProperty("HADOOP_USER_NAME", HADOOP_USER_NAME);

        //setup for append
        // conf.setBoolean("dfs.support.append", true);
        // conf.setBoolean("dfs.client.block.write.replace-datanode-on-failure.policy", false);
        // conf.setBoolean("dfs.client.block.write.replace-datanode-on-failure.enable", false);

        //extra
        // conf.set("fs.defaultFS", "hdfs://hadoop-master:9000/");

        //for mvn
        conf.set("fs.hdfs.impl", org.apache.hadoop.hdfs.DistributedFileSystem.class.getName());
        conf.set("fs.file.impl", org.apache.hadoop.fs.LocalFileSystem.class.getName());

        // System.setProperty("hadoop.home.dir", "/");

        try {
            fs = FileSystem.get(URI.create(hdfsUri), conf);
        } catch (Exception e) {
            logger.error("hdfs error during connection");
            logger.error(e.toString());
            throw e;
        }
    }

    public ArrayList<Path> getListOfFiles(FileSystem fs, Path dirPath) throws IOException {
        ArrayList<Path> paths = new ArrayList<>();
        RemoteIterator<LocatedFileStatus> fileStatusListIterator = fs.listFiles(dirPath, true);
        while (fileStatusListIterator.hasNext()) {
            LocatedFileStatus fileStatus = fileStatusListIterator.next();
            paths.add(fileStatus.getPath());
        }
        return paths;
    }

    public void write(String path, String fileName, String fileContent) {
        // Create folder if not exists
        try {
            if (!fs.exists(new Path(path))) {
                fs.mkdirs(new Path(path));
                logger.info("Path: " + path + " was created.");
            }
        } catch (Exception e) {
            logger.error(e.toString());
        }

        String timestamp = new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss_SSS'_'").format(new Date());
        fileName = timestamp + fileName;
        Path hdfsWritePath = new Path(path + "/" + fileName);
        logger.info(String.format("Begin write %s into hdfs", path));
        try {
            FSDataOutputStream outputStream = fs.create(hdfsWritePath);
            outputStream.writeBytes(fileContent);
            outputStream.close();
            logger.info(String.format("End write file into hdfs", path));
        } catch (Exception e) {
            logger.error(e.toString());
        }
    }

    public String read(Path fullPath) {
        logger.info(String.format("Read %s from hdfs", fullPath.toString()));
        String out = null;
        try {
            FSDataInputStream inputStream = fs.open(fullPath);
            out = IOUtils.toString(inputStream, "UTF-8");
            logger.info(String.format("End read %s from hdfs", fullPath.toString()));
            inputStream.close();
        } catch (Exception e) {
            logger.error(e.toString());
        }
        return out;
    }

    public String read(String path, String fileName) {
        return read(new Path(path + "/" + fileName));
    }

    private void checkSeries(String seriesName, String dataColumn, String qualityColumn, String filePath, String fileName) {
        int setsNumb = 0;
        QueryResult queryResult = Main.influxDBConnector.getInfluxDB().query(
                new Query(String.format("SELECT count(\"%s\") FROM \"%s\"", dataColumn, seriesName)));
        try {
            setsNumb = (int) (double) queryResult.getResults().get(0).getSeries().get(0).getValues().get(0).get(1);
        } catch (Exception e) {
            logger.warn("cannot init influx db / db series does not exist");
            return;
        }
        if (setsNumb >= maxNumbColorDataSetsInDB) {
            queryResult = Main.influxDBConnector.getInfluxDB().query(
                    new Query(String.format("SELECT \"%s\", \"%s\" FROM \"%s\"",
                            qualityColumn, dataColumn, seriesName)));

            List<List<Object>> tableData = queryResult.getResults().get(0).getSeries().get(0).getValues();
            StringBuilder stringBuilder = new StringBuilder();
            for (List<Object> tupel : tableData) {
                for (Object elem : tupel) {
                    stringBuilder.append(elem);
                    stringBuilder.append(";");
                }
                stringBuilder.append("\n");
            }

            write(filePath, fileName, stringBuilder.toString());

            //clean up db
            Main.influxDBConnector.getInfluxDB().query(
                    new Query(String.format("DROP SERIES FROM \"%s\"", seriesName)));
        }
    }

    @Override
    public void run() {
        if (isSimulation) {
            logger.info("Start checkSeries");
            while (running) {
                checkSeries(colorSourceName, DATA_FIELD, QUALITY_FIELD,
                        colorDataDir, colorDataFile);
                try {
                    Thread.sleep(checkSeriesIntervalSec * 100);
                } catch (InterruptedException e) {
                }
            }
            logger.info("End checkSeries");
        } else {
            //quality data was already collected
            BufferedReader br = null;
            try {
                br = new BufferedReader(new FileReader(COLLECTED_COLOR_DATA_FILE));
                String st = null;
                StringBuilder stringBuilder = new StringBuilder();
                while ((st = br.readLine()) != null) {
                    stringBuilder.append(st);
                    stringBuilder.append("\n");
                }
                br.close();
                write(colorDataDir, colorDataFile, stringBuilder.toString());
            } catch (Exception ex) {
                logger.error("cannot read colordata");
                logger.error(ex);
            }
            while (running) {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                }
            }
        }
    }

    public void closeFS() throws IOException {
        fs.close();
        running = false;
    }


}
