package mapReduce;

import hdfs.HDFSWritter;
import lombok.Getter;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.lib.input.MultipleInputs;
import org.apache.hadoop.mapreduce.lib.input.TextInputFormat;
import org.apache.hadoop.mapreduce.lib.output.TextOutputFormat;
import org.apache.log4j.Logger;

import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.concurrent.TimeUnit;

import static flink.Main.isSimulation;

@Getter
public class MapReduceJob implements Runnable {
    public static final int minNumbOfSetsForML = 150;
    public static volatile boolean isDataSetReady = false;
    private static final int writeStatsIntervalMin = 1;
    private static final int checkNumberOfSetsIntervalMin = 1;
    private static final String OUTPUT_DIR = "/user/colorDataStats/";
    private static Logger logger = Logger.getLogger(MapReduceJob.class);


    private HDFSWritter hdfsWritter;
    private volatile String currOutputDir;
    private volatile boolean running = false;

    public MapReduceJob(HDFSWritter hdfsWritter) {
        this.hdfsWritter = hdfsWritter;
        currOutputDir = "";
    }

    @Override
    public void run() {
        //only in simulation are enough data to mapReduce
        if(isSimulation) {
            running = true;
            new Thread(() -> {
                try {
                    logger.info("start checkNumberOfSets");
                    checkNumberOfSets();
                } catch (InterruptedException e) {
                    logger.error("problem during check numb of colorData sets");
                    logger.error(e);
                }
            }).start();


            while (!isDataSetReady) {
                logger.info("(re)start mapreduce thread");
                Configuration conf = new Configuration();
                Job job = null;
                try {
                    job = new Job(conf, "ColorDataMapReduce");
                } catch (IOException e) {
                    logger.error("cannot start hadoop job");
                    logger.error(e);
                }
                job.setJarByClass(MapReduceJob.class);
                job.setReducerClass(ColorDataReducer.class);
                job.setOutputKeyClass(Text.class);
                job.setOutputValueClass(Text.class);
                try {
                    ArrayList<Path> paths = hdfsWritter.getListOfFiles(hdfsWritter.getFs(), new Path(hdfsWritter.getColorDataDir()));
                    for (Path dataPath : paths) {
                        MultipleInputs.addInputPath(job, dataPath, TextInputFormat.class, ColorDataMapper.class);
                    }
                } catch (Exception e) {
                    //no data yet
                    logger.warn("data not found");
                    logger.warn(e);
                    try {
                        TimeUnit.MINUTES.sleep(1);
                    } catch (InterruptedException exception) {
                    }
                    continue;
                }
                currOutputDir = hdfsWritter.getHdfsUri() + OUTPUT_DIR +
                        new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss_SSS").format(new Date());
                TextOutputFormat.setOutputPath(job,
                        new Path(currOutputDir));
                try {
                    job.waitForCompletion(true);
                } catch (Exception e) {
                    logger.error("problem mapReduce");
                    logger.error(e);
                }
                try {
                    TimeUnit.MINUTES.sleep(writeStatsIntervalMin);
                } catch (InterruptedException e) {
                }
            }

            while (running) {
                //idel
                try {
                    TimeUnit.MINUTES.sleep(1);
                } catch (InterruptedException e) {
                }
            }
        }
    }

    private void checkNumberOfSets() throws InterruptedException {
        QualityStatistic qualityStatistic = new QualityStatistic();
        while (!isDataSetReady) {
            if (!currOutputDir.isEmpty()) {
                qualityStatistic = parseStatsFile(new Path(currOutputDir));
            }
            if (qualityStatistic.getGood() >= minNumbOfSetsForML &&
                    qualityStatistic.getDefect1() >= minNumbOfSetsForML &&
                    qualityStatistic.getDefect2() >= minNumbOfSetsForML) {
                isDataSetReady = true;
            }
            TimeUnit.MINUTES.sleep(checkNumberOfSetsIntervalMin);
        }
        logger.info("READY TO USE ML FOR COLORDATA PREDICTION");
    }


    public QualityStatistic parseStatsFile(Path path) {
        QualityStatistic qualityStatistic = new QualityStatistic();
        String lines = hdfsWritter.read(path.toString(), "part-r-00000");
        if (lines == null) {
            return qualityStatistic;
        }
        String[] timePeriods = lines.split("\n");
        for (String tupel : timePeriods) {
            String[] elems = tupel.substring(tupel.indexOf("\t") + 1).split(" ");
            for (String el : elems) {
                qualityStatistic = getQualityStatObj(qualityStatistic, el);
            }
        }
        return qualityStatistic;
    }

    private QualityStatistic getQualityStatObj(QualityStatistic qs, String str) {
        str = str.replace(",", "");
        try {
            if (str.contains(QualityStatistic.GOOD)) {
                str = str.replace(QualityStatistic.GOOD + "=", "");
                qs.setGood(qs.getGood() + Integer.parseInt(str));
            } else if (str.contains(QualityStatistic.DEFECT_1)) {
                str = str.replace(QualityStatistic.DEFECT_1 + "=", "");
                qs.setDefect1(qs.getDefect1() + Integer.parseInt(str));
            } else if (str.contains(QualityStatistic.DEFECT_2)) {
                str = str.replace(QualityStatistic.DEFECT_2 + "=", "");
                qs.setDefect2(qs.getDefect2() + Integer.parseInt(str));
            }
        } catch (Exception e) {
            logger.error("cannot parse int from stat file");
            logger.error(e);
        }
        return qs;
    }

    public void close() {
        running = false;
    }
}
