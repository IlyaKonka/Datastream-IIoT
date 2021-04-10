package datasimulator;

import lombok.Getter;
import org.apache.log4j.Logger;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;

import java.util.Properties;

@Getter
public class ParentSimulator {
    // ----- Required property keys
    public static final String URL = "URL";
    public static final String PORT = "PORT";
    public static final String SENSOR_ID = "SENSOR_ID";
    public static final String TOPIC = "TOPIC";

    // ------ Optional property keys
    public static final String USERNAME = "USERNAME";
    public static final String PASSWORD = "PASSWORD";

    // ------ Runtime fields
    private transient MqttClient client;
    private Properties properties;
    private Logger logger;

    public ParentSimulator(Properties properties, Logger logger) {
        this.properties = properties;
        this.logger = logger;
    }

    public void connect() {
        MqttConnectOptions connectOptions = new MqttConnectOptions();
        connectOptions.setCleanSession(true);
        if (properties.containsKey(USERNAME)) {
            connectOptions.setUserName(properties.getProperty(USERNAME));
        }
        if (properties.containsKey(PASSWORD)) {
            connectOptions.setPassword(properties.getProperty(PASSWORD).toCharArray());
        }
        connectOptions.setAutomaticReconnect(true);

        try {
            client = new MqttClient(properties.getProperty(URL) + ":" + properties.getProperty(PORT),
                    properties.getProperty(SENSOR_ID));
        } catch (MqttException e) {
            logger.error(e);
        }
        try {
            client.connect(connectOptions);
        } catch (MqttException e) {
            logger.error(e);
        }

    }
}
