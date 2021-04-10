package datasimulator;

import org.apache.log4j.Logger;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import java.nio.charset.StandardCharsets;
import java.util.Properties;
import java.util.Random;

// Publishes randomly generated vibration every 500 milliseconds
public class VibrationSensorSimulator extends ParentSimulator implements Runnable {
    private static Logger logger = Logger.getLogger(VibrationSensorSimulator.class);

    public VibrationSensorSimulator(Properties properties) {
        super(properties, logger);
    }

    @Override
    public void run() {
        connect();

        // Generate random vibration (x,y,z) 0...2
        Random rand = new Random();
        while (true) {
            double vibrationX = rand.nextDouble() * 2.;
            double vibrationY = rand.nextDouble() * 2.;
            double vibrationZ = rand.nextDouble() * 2.;
            logger.info("vibrationX: " + vibrationX);
            logger.info("vibrationY: " + vibrationY);
            logger.info("vibrationZ: " + vibrationZ);
            try {
                getClient().publish(getProperties().getProperty(ParentSimulator.TOPIC),
                        new MqttMessage((String.valueOf(vibrationX) + ";" + String.valueOf(vibrationY) + ";" +
                                String.valueOf(vibrationZ)).getBytes(StandardCharsets.UTF_8)));
            } catch (MqttException e) {
                logger.error(e.toString());
            }
            try {
                Thread.sleep(500);
            } catch (Exception ex) {
            }
        }
    }

}
