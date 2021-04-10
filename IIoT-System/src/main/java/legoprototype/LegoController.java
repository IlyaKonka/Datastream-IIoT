package legoprototype;

import flink.Main;
import org.apache.log4j.Logger;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import topics.*;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;

import static flink.Main.knnPrediction;

public class LegoController implements Runnable {
    private static Logger logger = Logger.getLogger(LegoController.class);

    private static HashMap<String, String> rfidAndThickness;

    static {
        rfidAndThickness = new HashMap<>();
        rfidAndThickness.put("80795adb", "50");
        rfidAndThickness.put("d0715adb", "50");
        rfidAndThickness.put("40e35bdb", "50");
        rfidAndThickness.put("d0655adb", "50");
        rfidAndThickness.put("e02f5ddb", "50");
        rfidAndThickness.put("e0195adb", "50");
        rfidAndThickness.put("801e5adb", "50");
    }

    private boolean isLegoPrototypeActive = false;
    private boolean running = false;
    private ArrayList<Material> currMaterials = new ArrayList<>();

    @Override
    public void run() {

        logger.info("lego controller was started");

        if (Main.mqttClient.getClient() == null) {
            logger.error("Please init mqtt client in Main.class at first");
        } else {
            running = true;
            try {
                // === COMMON ===
                Main.mqttClient.getClient().subscribe(CommonTopic.TABLES_CONNECTION, (topic, message) -> {
                    logger.info(new String(message.getPayload(), StandardCharsets.UTF_8));
                });

                // === LOADING ===
                Main.mqttClient.getClient().subscribe(LoadingTopic.READY_TO_NEW_MAT, (topic, message) -> {
                    if (!isLegoPrototypeActive) {
                        logger.info("lego prototype is connected (heartbeat)");
                    }
                });
                Main.mqttClient.getClient().subscribe(LoadingTopic.MAT_LOADED, (topic, message) -> {
                    isLegoPrototypeActive = true;
                    String currMat = new String(message.getPayload(), StandardCharsets.UTF_8);
                    if (rfidAndThickness.containsKey(currMat)) { //forward
                        Main.mqttClient.getClient().publish(LoadingTopic.MAT_PROCESS,
                                new MqttMessage((currMat + "|F").getBytes(StandardCharsets.UTF_8)));
                        currMaterials.add(new Material(currMat));
                        logger.info(String.format("forward %s material", currMat));
                    } else { //drop
                        Main.mqttClient.getClient().publish(LoadingTopic.MAT_PROCESS,
                                new MqttMessage((currMat + "|D").getBytes(StandardCharsets.UTF_8)));
                        logger.info(String.format("drop %s material", currMat));
                    }
                });
                Main.mqttClient.getClient().subscribe(LoadingTopic.MAT_POSITION, (topic, message) -> {
                    positionUpdate(new String(message.getPayload(), StandardCharsets.UTF_8), "loading");
                });
                Main.mqttClient.getClient().subscribe(LoadingTopic.MAT_DROPPED, (topic, message) -> {
                    String currMat = new String(message.getPayload(), StandardCharsets.UTF_8);
                    logger.info(String.format("material %s was dropped", currMat));
                });
                Main.mqttClient.getClient().subscribe(LoadingTopic.RFID_READ_PROBLEM, (topic, message) -> {
                    logger.warn("Loading: " + new String(message.getPayload(), StandardCharsets.UTF_8));
                });
                Main.mqttClient.getClient().subscribe(LoadingTopic.WARNING, (topic, message) -> {
                    logger.warn("Loading: " + new String(message.getPayload(), StandardCharsets.UTF_8));
                });
//                Main.mqttClient.getClient().subscribe(LoadingTopic.INFO, (topic, message) -> {
//                    logger.info("Loading: " + new String(message.getPayload(), StandardCharsets.UTF_8));
//                });


                // === ROLLING ===
                Main.mqttClient.getClient().subscribe(RollingTopic.ROLL_QUERY, (topic, message) -> {
                    String currMat = new String(message.getPayload(), StandardCharsets.UTF_8);
                    if (getMaterialObj(currMat) == null) {
                        logger.error(String.format("can not roll unknown material %s", currMat));
                    } else {
                        Main.mqttClient.getClient().publish(RollingTopic.ROLL_THICKNESS,
                                new MqttMessage((currMat + "|" + rfidAndThickness.get(currMat))
                                        .getBytes(StandardCharsets.UTF_8)));
                        logger.info(String.format("Roll material %s with thickness %s", currMat,
                                rfidAndThickness.get(currMat)));
                    }
                });
                Main.mqttClient.getClient().subscribe(RollingTopic.MAT_POSITION, (topic, message) -> {
                    positionUpdate(new String(message.getPayload(), StandardCharsets.UTF_8), "rolling");
                });
                Main.mqttClient.getClient().subscribe(RollingTopic.WARNING, (topic, message) -> {
                    logger.warn("Rolling: " + new String(message.getPayload(), StandardCharsets.UTF_8));
                });
//                Main.mqttClient.getClient().subscribe(RollingTopic.INFO, (topic, message) -> {
//                    logger.info("Rolling: " + new String(message.getPayload(), StandardCharsets.UTF_8));
//                });
                Main.mqttClient.getClient().subscribe(RollingTopic.MAT_AFTER_ROLL, (topic, message) -> {
                    String currMat = new String(message.getPayload(), StandardCharsets.UTF_8);
                    getMaterialObj(currMat).setRolled(true);
                    logger.info(String.format("Material %s rolled successfully", currMat));
                });


                // === QUALITY ===
                Main.mqttClient.getClient().subscribe(QualityTopic.MAT_READY_TO_QCHECK, (topic, message) -> {
                    String currMat = new String(message.getPayload(), StandardCharsets.UTF_8);
                    if (getMaterialObj(currMat) == null || !getMaterialObj(currMat).isRolled()) {
                        logger.error(String.format("material %s is not ready to quality check", currMat));
                    } else {
                        logger.info(String.format("material %s is ready to quality check", currMat));
                        //server verification
                        getMaterialObj(currMat).setReadyToQCheck(true);
                        Main.mqttClient.getClient().publish(ColorSensorTopic.MAT_READY_TO_QCHECK,
                                new MqttMessage(currMat.getBytes(StandardCharsets.UTF_8)));
                    }
                });
                Main.mqttClient.getClient().subscribe(QualityTopic.MAT_POSITION, (topic, message) -> {
                    positionUpdate(new String(message.getPayload(), StandardCharsets.UTF_8), "quality");
                });
                Main.mqttClient.getClient().subscribe(QualityTopic.WARNING, (topic, message) -> {
                    logger.warn("Quality: " + new String(message.getPayload(), StandardCharsets.UTF_8));
                });
//                Main.mqttClient.getClient().subscribe(QualityTopic.INFO, (topic, message) -> {
//                    logger.info("Quality: " + new String(message.getPayload(), StandardCharsets.UTF_8));
//                });


                // === COLOR SENSOR ===
                Main.mqttClient.getClient().subscribe(ColorSensorTopic.MAT_CHECK, (topic, message) -> {
                    try {
                        byte[] currMatAndData = message.getPayload();
                        byte[] currMatByteArr = new byte[8];
                        System.arraycopy(currMatAndData, 0, currMatByteArr, 0, 8);
                        String currMat = new String(currMatByteArr, StandardCharsets.UTF_8);
                        if (currMat.equals("")) {
                            //skip
                        } else if (!getMaterialObj(currMat).isReadyToQCheck()) {
                            logger.error(String.format("Material %s is not ready to quality check", currMat));
                        } else {
                            String data = null;
                            try {
                                data = Arrays.toString(convert(currMatAndData))
                                        .replaceAll(" ", "")
                                        .replace("[", "")
                                        .replace("]", "");

                                //System.out.println(String.format("ColorData: %s", data));
                            } catch (Exception e) {
                                logger.error("cannot parse data");
                            }
                            try {
                                if (data != null) {
                                    data = ";" + data;
                                    if (knnPrediction.getKnn() == null) {
                                        logger.error("KNN is not ready add delay");
                                    }
                                    String prediction = knnPrediction.makePrediction(data, false);
                                    logger.info("Quality prediction: " +
                                            prediction.replace("probability ", ""));
                                    getMaterialObj(currMat).setQuality(parseProbabilityData(prediction));
                                }
                            } catch (Exception e) {
                                logger.error("cannot make quality prediction");
                            }
                            Main.mqttClient.getClient().publish(QualityTopic.MAT_CHECK,
                                    new MqttMessage((currMat + "|0").getBytes(StandardCharsets.UTF_8)));
                            logger.info(String.format("Material %s classified as %s", currMat,
                                    getMaterialObj(currMat).getQuality()));
                        }
                    } catch (Exception ex) {
                        //logger.warn("empty msg ColorSensorTopic.MAT_CHECK");
                        //logger.warn(ex);
                    }
                });
                Main.mqttClient.getClient().subscribe(ColorSensorTopic.WARNING, (topic, message) -> {
                    logger.warn("ColorSensor: " + new String(message.getPayload(), StandardCharsets.UTF_8));
                });


                // === UNLOADING ===
                Main.mqttClient.getClient().subscribe(UnloadingTopic.MAT_QUERY, (topic, message) -> {
                    String currMat = new String(message.getPayload(), StandardCharsets.UTF_8);
                    if (getMaterialObj(currMat) != null && getMaterialObj(currMat).getQuality() != null) {
                        if (getMaterialObj(currMat).getQuality().equals("good")) {
                            Main.mqttClient.getClient().publish(UnloadingTopic.MAT_DROP,
                                    new MqttMessage((currMat + "|0").getBytes(StandardCharsets.UTF_8)));
                            logger.info(String.format("Forward material %s to non-defective zone", currMat));
                        } else {
                            Main.mqttClient.getClient().publish(UnloadingTopic.MAT_DROP,
                                    new MqttMessage((currMat + "|1").getBytes(StandardCharsets.UTF_8)));
                            logger.info(String.format("Drop material %s into defect zone", currMat));
                        }
                    }
                });
                Main.mqttClient.getClient().subscribe(UnloadingTopic.MAT_FINAL, (topic, message) -> {
                    String currMat = new String(message.getPayload(), StandardCharsets.UTF_8);
                    if (getMaterialObj(currMat) != null && getMaterialObj(currMat).getQuality() != null) {
                        if (getMaterialObj(currMat).getQuality().equals("good")) {
                            logger.info(String.format("Material %s in non-defective zone", currMat));
                        } else {
                            logger.info(String.format("Material %s in defect zone", currMat));
                        }
                        rfidAndThickness.remove(currMat); //del from map, next time drop
                        isLegoPrototypeActive = false;
                    }
                });
                Main.mqttClient.getClient().subscribe(UnloadingTopic.MAT_POSITION, (topic, message) -> {
                    positionUpdate(new String(message.getPayload(), StandardCharsets.UTF_8), "unloading");
                });
                Main.mqttClient.getClient().subscribe(UnloadingTopic.WARNING, (topic, message) -> {
                    logger.warn("Unloading: " + new String(message.getPayload(), StandardCharsets.UTF_8));
                });
//                Main.mqttClient.getClient().subscribe(UnloadingTopic.INFO, (topic, message) -> {
//                    logger.info("Unloading: " + new String(message.getPayload(), StandardCharsets.UTF_8));
//                });
//                Main.mqttClient.getClient().subscribe(UnloadingTopic.MOTOR_INFO, (topic, message) -> {
//                   logger.info("Unloading: " + new String(message.getPayload(), StandardCharsets.UTF_8));
//                });


                while (running) {
                    //idel
                    Thread.sleep(100);
                }

            } catch (MqttException | InterruptedException e) {
                logger.error("problem during subscribing");
                logger.error(e);
            }

        }

    }

    private String parseProbabilityData(String prediction) {
        prediction = prediction.replaceAll("}", "").replaceAll("\\{", "")
                .replaceAll("probability ", "").replaceAll(",", "");
        String[] qualities = prediction.split(" ");
        double defect2 = 0.0;
        double defect1 = 0.0;
        double good = 0.0;
        try {
            for (String q : qualities) {
                double val = Double.parseDouble(q.substring(q.indexOf("=") + 1));
                if (q.contains("defect2")) {
                    defect2 = val;
                } else if (q.contains("defect1")) {
                    defect1 = val;
                } else if (q.contains("good")) {
                    good = val;
                }
            }
        } catch (Exception e) {
            logger.error(e);
            logger.error("cannot parse probability res");
            return "unknown";
        }

        if (Double.compare(defect1, defect2) >= 0 && Double.compare(defect1, good) >= 0) {
            return "defect1";
        } else if (Double.compare(defect2, defect1) >= 0 && Double.compare(defect2, good) >= 0) {
            return "defect2";
        } else {
            return "good";
        }
    }

    private void positionUpdate(String currMatAndLocation, String loc) {
        String currMat = currMatAndLocation.split("\\|")[0];
        String currLocation = currMatAndLocation.split("\\|")[1];
        if (getMaterialObj(currMat) == null) {
            if (!loc.equals("loading")) {
                logger.error(String.format("unknown material %s", currMat));
            }
        } else {
            getMaterialObj(currMat).setLocation(currLocation);
            logger.info(String.format("Material: %s Location: %s Position: %s", currMat, loc, currLocation));
        }
    }

    private Material getMaterialObj(String currMat) {
        for (Material m : currMaterials) {
            if (m.getId().equals(currMat)) {
                return m;
            }
        }
        return null;
    }

    private int[] convert(byte[] byteArr) {
        int[] unsignedArr = new int[byteArr.length - 8];
        for (int i = 0; i < byteArr.length - 8; i++) {
            unsignedArr[i] = byteArr[i + 8] & 0xFF;
        }
        return unsignedArr;
    }

    private void close() {
        running = false;
    }
}
