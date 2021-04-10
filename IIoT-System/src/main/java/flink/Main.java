package flink;

import ML.KNNPrediction;
import datasimulator.ColorSensorSimulator;
import datasimulator.VibrationSensorSimulator;
import hdfs.HDFSWritter;
import influxdb.ColorDataSink;
import influxdb.InfluxDBConnector;
import influxdb.VibrationSink;
import legoprototype.LegoController;
import mapReduce.MapReduceJob;
import mqtt.ColorTrainDataSource;
import mqtt.CustomMQTTClient;
import mqtt.VibrationSource;
import org.apache.flink.api.java.tuple.Tuple4;
import org.apache.flink.api.java.tuple.Tuple5;
import org.apache.flink.streaming.api.datastream.DataStream;
import org.apache.flink.streaming.api.environment.StreamExecutionEnvironment;
import org.apache.flink.streaming.api.functions.timestamps.AscendingTimestampExtractor;
import org.apache.flink.streaming.api.functions.windowing.delta.DeltaFunction;
import org.apache.flink.streaming.api.windowing.time.Time;
import org.apache.flink.streaming.api.windowing.triggers.DeltaTrigger;
import org.apache.log4j.DailyRollingFileAppender;
import org.apache.log4j.Level;
import org.apache.log4j.Logger;
import org.apache.log4j.PatternLayout;
import org.influxdb.dto.Query;
import vibration.MotorController;
import vibration.VibrationAnomalySink;

import java.io.FileInputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Properties;

public class Main {
    private static final Logger logger = Logger.getRootLogger();
    public static final SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss.SSS");

    private static final String MQTT_PROP = "src/main/resources/mqttClient.properties";
    private static final String DB_PROP = "src/main/resources/db.properties";
    private static final String VIB_SIMULATOR_PROP = "src/main/resources/mqttVibrationSensor.properties";
    private static final String VIB_SIMULATOR_SOURCE_NAME = "/vibrationSensor/vibration";
    private static final String COLOR_SIMULATOR_PROP = "src/main/resources/mqttColorSensor.properties";
    private static final String COLOR_SIMULATOR_SOURCE_NAME = "fakeColorData";

    private static final String VIB_SOURCE_NAME = "/vibrationSensor/vibration";
    private static final String COLOR_SOURCE_NAME = "/colourSensor/colourData";

    private static final String HDFS_URI = "hdfs://172.30.0.7:9000";

    private static final String COLOR_DATA_DIR = "/main/colorData/";
    private static final String COLOR_DATA_FILE = "dataset.txt";

    public static final String COLLECTED_COLOR_DATA_FILE = "src/main/resources/colorData.txt";

    public static String colorSourceName;
    public static String vibrationSourceName;
    public static CustomMQTTClient mqttClient;
    public static InfluxDBConnector influxDBConnector;
    public static MotorController motorController;
    public static KNNPrediction knnPrediction;
    public static double xyzAvgDiff = 0;

    public static boolean isSimulation = true;
    public static int waitTime = 60;

    public static void main(String[] args) {
        parseArgs(args);
        logger.setLevel(Level.INFO);
        wait(waitTime); //waiting for other docker containers

        Properties mqttClientProperties = null;
        Properties dbProperties = null;
        ColorSensorSimulator colorSensorSimulator = null;

        if (isSimulation) {
            colorSourceName = COLOR_SIMULATOR_SOURCE_NAME;
            vibrationSourceName = VIB_SIMULATOR_SOURCE_NAME;
        } else {
            colorSourceName = COLOR_SOURCE_NAME;
            vibrationSourceName = VIB_SOURCE_NAME;
        }

        //init mqtt client
        try {
            mqttClientProperties = new Properties();
            mqttClientProperties.load(new FileInputStream(MQTT_PROP));
            mqttClient = new CustomMQTTClient(mqttClientProperties);
            mqttClient.init();
        } catch (Exception e) {
            logger.error("cannot init mqttClient");
            logger.error(e);
        }

        //init influx db
        try {
            dbProperties = new Properties();
            dbProperties.load(new FileInputStream(DB_PROP));
            influxDBConnector = new InfluxDBConnector(dbProperties);
            influxDBConnector.connect();
        } catch (Exception e) {
            logger.error("cannot init influx db");
            logger.error(e);
        }

        //cleanup DB
        influxDBConnector.getInfluxDB().query(
                new Query(String.format("DROP SERIES FROM \"%s\"", colorSourceName)));
        influxDBConnector.getInfluxDB().query(
                new Query(String.format("DROP SERIES FROM \"%s\"", vibrationSourceName)));


        if (isSimulation) {
            //===SIMULATION VIBRATION SENSOR===
            try {
                logger.info("start vibration sensor simulator");
                Properties vibSensProp = new Properties();
                vibSensProp.load(new FileInputStream(VIB_SIMULATOR_PROP));
                new Thread(new VibrationSensorSimulator(vibSensProp)).start();
            } catch (Exception e) {
                logger.error("cannot read vibSensor properties");
                logger.error(e);
            }

            //===SIMULATION COLOR SENSOR===
            try {
                logger.info("start color sensor simulator");
                Properties colSensProp = new Properties();
                colSensProp.load(new FileInputStream(COLOR_SIMULATOR_PROP));
                colorSensorSimulator = new ColorSensorSimulator(colSensProp);
                new Thread(colorSensorSimulator).start();
            } catch (Exception e) {
                logger.error("cannot read colSensor properties");
                logger.error(e);
            }

        }


        //===HDFS Thread===
        HDFSWritter hdfsWritter = new HDFSWritter(HDFS_URI, COLOR_DATA_DIR, COLOR_DATA_FILE);
        try {
            new Thread(hdfsWritter).start();
        } catch (Exception e) {
            logger.error("cannot start hdfs thread");
            logger.error(e);
        }


        //===MapReduce Thread===
        try {
            new Thread(new MapReduceJob(hdfsWritter)).start();
        } catch (Exception e) {
            logger.error("cannot start mapReduce thread");
            logger.error(e);
        }


        //===KNNPrediction Thread===
        try {
            knnPrediction = new KNNPrediction(hdfsWritter, colorSensorSimulator);
            new Thread(knnPrediction).start();
        } catch (Exception e) {
            logger.error("cannot start KNNPrediction thread");
            logger.error(e);
        }


        //===LEGO Controller Thread===
        try {
            new Thread(new LegoController()).start();
        } catch (Exception e) {
            logger.error("cannot start Lego Controller thread");
            logger.error(e);
        }


        //===MOTOR Controller Thread===
        try {
            motorController = new MotorController();
            new Thread(motorController).start();
        } catch (Exception e) {
            logger.error("cannot start Lego Controller thread");
            logger.error(e);
        }

        // obtain execution environment and set setBufferTimeout to 1 to enable
        // continuous flushing of the output buffers (lowest latency)
        final StreamExecutionEnvironment env =
                StreamExecutionEnvironment.getExecutionEnvironment().setBufferTimeout(1);

        // Enable fault-tolerance for state
        //env.enableCheckpointing(1000);

        // Enable Event Time processing
        //env.setStreamTimeCharacteristic(TimeCharacteristic.EventTime);

        // Add mqtt vibration source
        VibrationSource vibrationSource = new VibrationSource(vibrationSourceName);
        DataStream<Tuple5<String, Double, Double, Double, Long>> mqttVibrationDataSource =
                env.addSource(vibrationSource);


        double anomalyThreshold = 0.6; //empirical coeff
        DataStream<Tuple5<String, Double, Double, Double, Long>> vibrationAnomalyStream =
                mqttVibrationDataSource.assignTimestampsAndWatermarks(new CustomTimestamp())
                        .keyBy(0)
                        .timeWindow(Time.milliseconds(3000))
                        .trigger(DeltaTrigger.of(anomalyThreshold,
                                new DeltaFunction<Tuple5<String, Double, Double, Double, Long>>() {
                                    private static final long serialVersionUID = 1L;

                                    @Override
                                    public double getDelta(
                                            Tuple5<String, Double, Double, Double, Long> oldDataPoint,
                                            Tuple5<String, Double, Double, Double, Long> newDataPoint) {

                                        xyzAvgDiff = (Math.abs(oldDataPoint.f1 - newDataPoint.f1) +
                                                Math.abs(oldDataPoint.f2 - newDataPoint.f2) +
                                                Math.abs(oldDataPoint.f3 - newDataPoint.f3)) / 3.0;
                                        double xyzAvgCurr = (Math.abs(newDataPoint.f1) +
                                                Math.abs(newDataPoint.f2) +
                                                Math.abs(newDataPoint.f3)) / 3.0;

                                        if (motorController.isRolling() &&
                                                Double.compare(xyzAvgCurr, motorController.vibrationLimit) < 0) {
                                            return 1; //trigger
                                        }
                                        return xyzAvgDiff;
                                    }
                                }, mqttVibrationDataSource.getType().createSerializer(env.getConfig())))
                        .maxBy(4);


        //configured only for real sensor
        if (!isSimulation) {
            // Add sink to monitor anomalies
            VibrationAnomalySink vibrationAnomalySink = new VibrationAnomalySink();
            vibrationAnomalyStream.addSink(vibrationAnomalySink);
        }


        // Writes vibration sensor stream to InfluxDB
        VibrationSink vibrationSink = new VibrationSink();
        mqttVibrationDataSource.addSink(vibrationSink);


        // Add mqtt colordata source
        ColorTrainDataSource colorTrainDataSource = new ColorTrainDataSource(colorSourceName);
        DataStream<Tuple4<String, String, String, Long>> mqttColorDataSource = env.addSource(colorTrainDataSource);

        // Writes color sensor stream to InfluxDB
        ColorDataSink colorDataSink = new ColorDataSink();
        mqttColorDataSource.addSink(colorDataSink);


        try {
            env.execute("flink job");
        } catch (Exception e) {
            logger.error("cannot start flink job");
            logger.error(e);
        }

    }

    private static void parseArgs(String[] args) {
        try {
            waitTime = Integer.parseInt(args[0]);
        } catch (Exception e) {
            logger.warn("cannot parse waitTime from arg[0]");
        }
        try {
            isSimulation = Boolean.parseBoolean(args[1]);
        } catch (Exception e) {
            logger.warn("cannot parse isSimulation from arg[1]");
        }
    }

    public static void wait(int sec) {
        for (int i = sec; i > 0; i--) {
            logger.info("Wait: " + i + " sec");
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
            }
        }
    }

    private static void writeLogFile() throws IOException {
        PatternLayout layout = new PatternLayout("%d{ISO8601} %-5p [%t] %c: %m%n");
        DailyRollingFileAppender fileAppender =
                new DailyRollingFileAppender(layout, "logs/logfile.log", "'.'yyyy-MM-dd_HH-mm");
        logger.addAppender(fileAppender);
    }

    private static class CustomTimestamp extends AscendingTimestampExtractor<Tuple5<String, Double, Double, Double, Long>> {
        private static final long serialVersionUID = 1L;

        @Override
        public long extractAscendingTimestamp(Tuple5<String, Double, Double, Double, Long> elem) {
            return elem.f4;
        }
    }

}

