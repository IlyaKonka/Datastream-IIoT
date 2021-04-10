package ML;

import datasimulator.ColorSensorSimulator;
import hdfs.HDFSWritter;
import lombok.Getter;
import mapReduce.MapReduceJob;
import mapReduce.QualityStatistic;
import net.sf.javaml.classification.Classifier;
import net.sf.javaml.classification.KNearestNeighbors;
import net.sf.javaml.core.Dataset;
import net.sf.javaml.core.DefaultDataset;
import net.sf.javaml.core.DenseInstance;
import net.sf.javaml.core.Instance;
import org.apache.hadoop.fs.Path;
import org.apache.log4j.Logger;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.concurrent.TimeUnit;

@Getter
public class KNNPrediction implements Runnable {
    private static Logger logger = Logger.getLogger(KNNPrediction.class);
    private HDFSWritter hdfsWritter;
    private QualityStatistic qualityStatistic = new QualityStatistic();
    private Classifier knn;
    private boolean running = false;
    private ColorSensorSimulator css = null;

    public KNNPrediction(HDFSWritter hdfsWritter) {
        this.hdfsWritter = hdfsWritter;
    }

    public KNNPrediction(HDFSWritter hdfsWritter, ColorSensorSimulator css) {
        this.hdfsWritter = hdfsWritter;
        this.css = css;
    }

    @Override
    public void run() {
        running = true;

        if (css != null) {
            while (!MapReduceJob.isDataSetReady) {
                //wait
                try {
                    TimeUnit.SECONDS.sleep(5);
                } catch (InterruptedException e) {
                }
            }
        }

        try {
            try {
                TimeUnit.SECONDS.sleep(10); //wait for data from HDFS
            } catch (InterruptedException e) {
            }
            trainKNN();
        } catch (IOException e) {
            logger.error("cannot train KNN");
            logger.error(e);
        }


        // === RUN SIMULATION ===
        if (css != null) {
            while (running) {
                logger.info(makeFakePrediction(css));
                try {
                    TimeUnit.SECONDS.sleep(5);
                } catch (InterruptedException e) {
                }
            }
        } else {
            while (running) {
                try {
                    TimeUnit.SECONDS.sleep(1);
                } catch (InterruptedException e) {
                }
            }
        }
    }


    public String makeFakePrediction(ColorSensorSimulator css) {
        try {
            String fakeDataSet = css.getFakeDataSet();
            logger.info("Fake colordata set: " + fakeDataSet);
            return makePrediction(fakeDataSet, true);
        } catch (IOException e) {
            logger.error("cannot make fake prediction KNN");
            logger.error(e);
        }
        return null;
    }

    public String makePrediction(String dataSet, boolean isQualityExists) {
        Dataset dataForTest = new DefaultDataset();
        dataForTest = parseStringDataToDataset(dataForTest, dataSet, isQualityExists);
        String probability = "probability ";
        for (Instance inst : dataForTest) {
            probability += knn.classDistribution(inst);
        }
        return probability;
    }

    public void trainKNN() throws IOException {
        Dataset dataForTraining = new DefaultDataset();
        try {
            ArrayList<Path> paths =
                    hdfsWritter.getListOfFiles(hdfsWritter.getFs(), new Path(hdfsWritter.getColorDataDir()));
            for (Path dataPath : paths) {
                if (!checkNumbOfData()) {
                    break;
                }
                dataForTraining = parseStringDataToDataset(dataForTraining, hdfsWritter.read(dataPath), true);
            }
        } catch (Exception e) {
            logger.error("cannot read files from hdfs for KNN");
            logger.error(e);
        }

        knn = new KNearestNeighbors(5);
        knn.buildClassifier(dataForTraining);
    }

    // return true if not enough data, false if ready to knn
    private boolean checkNumbOfData() {
        if (qualityStatistic.getGood() > MapReduceJob.minNumbOfSetsForML &&
                qualityStatistic.getDefect1() > MapReduceJob.minNumbOfSetsForML &&
                qualityStatistic.getDefect2() > MapReduceJob.minNumbOfSetsForML) {
            return false;
        }
        return true;
    }

    private Dataset parseStringDataToDataset(Dataset dataSet, String set, boolean isQualityExists) {
        // tupel (timestamp;quality;dataSet)
        for (String tupel : set.split("\n")) {
            String withoutTimeStamp = tupel.substring(tupel.indexOf(";") + 1);
            if (isQualityExists) {
                String[] qualityAndData = withoutTimeStamp.split(";");
                double[] profData = Arrays.stream(qualityAndData[1].split(","))
                        .mapToDouble(Double::parseDouble).toArray();
                dataSet.add(new DenseInstance(profData, qualityAndData[0]));
                updateQualityStat(qualityAndData[0]);
            } else {
                double[] profData = Arrays.stream(withoutTimeStamp.split(","))
                        .mapToDouble(Double::parseDouble).toArray();
                dataSet.add(new DenseInstance(profData, "unknown"));
            }

        }
        return dataSet;
    }

    private void updateQualityStat(String quality) {
        if (quality.equals("good")) {
            qualityStatistic.setGood(qualityStatistic.getGood() + 1);
        } else if (quality.equals("defect1")) {
            qualityStatistic.setDefect1(qualityStatistic.getDefect1() + 1);
        } else if (quality.equals("defect2")) {
            qualityStatistic.setDefect2(qualityStatistic.getDefect2() + 1);
        }
    }

    public void close() {
        running = false;
    }

}
