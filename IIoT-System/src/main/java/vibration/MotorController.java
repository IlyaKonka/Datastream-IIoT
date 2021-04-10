package vibration;

import flink.Main;
import lombok.Getter;
import org.apache.log4j.Logger;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import topics.RollingTopic;

import java.nio.charset.StandardCharsets;


@Getter
public class MotorController implements Runnable {
    private static Logger logger = Logger.getLogger(MotorController.class);
    private boolean running = false;
    private volatile boolean rolling = false;
    private volatile boolean rollingTerminated = false;
    public double vibrationLimit = 0.3;
    private int anomalyCount = 0;

    @Override
    public void run() {

        logger.info("motor controller was started");


        if (Main.mqttClient.getClient() == null) {
            logger.error("Please init mqtt client in Main.class at first");
        } else {
            running = true;
            try {
                Main.mqttClient.getClient().subscribe(RollingTopic.MOTOR_INFO, (topic, message) -> {
                    String msg = new String(message.getPayload(), StandardCharsets.UTF_8);
                    //System.out.println("MSG:"+msg);
                    if (msg.startsWith("isRollingStarted")) {
                        rolling = msg.split("\\|")[1].equals("1");
                        //System.out.println("isRollingStarted:"+rolling);
                    }
                });


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


    public void checkAnomaly(double x, double y, double z) {
        if (!rollingTerminated) {
            double vibrationLvl = (Math.abs(x) + Math.abs(y) + Math.abs(z)) / 3.0;
            //System.out.println("vibrationLvl: "+vibrationLvl);
            if (rolling && Double.compare(vibrationLvl, vibrationLimit) < 0
                    && Double.compare(Main.xyzAvgDiff, vibrationLimit / 2.0) < 0) {
                anomalyCount++;
            } else {
                anomalyCount = 0;
            }
            if (anomalyCount > 1) {
                stopRolling();
                logger.error("ROLL WORKS INCORRECTLY");
                logger.error("STOP ROLLING");
                rollingTerminated = true;
                int waitSec = 10;
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        logger.info("Waiting before start rolling");
                        for (int i = waitSec; i > 0; i--) {
                            logger.info("Wait: " + i + " sec");
                            try {
                                Thread.sleep(1000);
                            } catch (InterruptedException e) {
                            }
                        }
                        logger.info("START ROLLING");
                        startRolling();
                        rollingTerminated = false;
                    }
                }).start();
            } else if (rolling) {
                logger.info("material is rolling");
            }
        }
    }

    public void startRolling() {
        try {
            Main.mqttClient.getClient().publish(RollingTopic.CONTROL,
                    new MqttMessage(("m").getBytes(StandardCharsets.UTF_8)));
            Main.mqttClient.getClient().publish(RollingTopic.CONTROL,
                    new MqttMessage(("s").getBytes(StandardCharsets.UTF_8)));
        } catch (MqttException e) {
            logger.error("problem during startRolling");
            logger.error(e);
        }
    }

    public void stopRolling() {
        try {
            Main.mqttClient.getClient().publish(RollingTopic.CONTROL,
                    new MqttMessage(("z").getBytes(StandardCharsets.UTF_8)));
            Main.mqttClient.getClient().publish(RollingTopic.CONTROL,
                    new MqttMessage(("p").getBytes(StandardCharsets.UTF_8)));
        } catch (MqttException e) {
            logger.error("problem during stopRolling");
            logger.error(e);
        }
    }

    private void close() {
        running = false;
    }
}
