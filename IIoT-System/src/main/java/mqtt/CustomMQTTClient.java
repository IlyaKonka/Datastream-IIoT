package mqtt;

import lombok.Getter;
import org.apache.log4j.Logger;
import org.eclipse.paho.client.mqttv3.*;

import java.io.Serializable;
import java.util.Properties;

@Getter
public class CustomMQTTClient implements Serializable {
    private static Logger logger = Logger.getLogger(CustomMQTTClient.class);
    private static final long serialVersionUID = 5835738559536126731L;

    // ----- Required property keys
    public static final String URL = "URL";
    public static final String PORT = "PORT";
    public static final String CLIENT_ID = "CLIENT_ID";

    // ------ Optional property keys
    public static final String USERNAME = "USERNAME";
    public static final String PASSWORD = "PASSWORD";

    private transient MqttClient client;
    private Properties properties;
    private MqttConnectOptions connectOptions;

    MqttCallback callback = new MqttCallback() {
        public void connectionLost(Throwable t) {
            try {
                client.connect(connectOptions);
                logger.warn("MQTT client reconnected");
            } catch (MqttException e) {
                logger.error("Cannot reconnect");
            }
        }

        @Override
        public void messageArrived(String s, MqttMessage mqttMessage) throws Exception {
        }

        @Override
        public void deliveryComplete(IMqttDeliveryToken iMqttDeliveryToken) {
        }
    };

    public CustomMQTTClient(Properties properties) {
        this.properties = properties;
    }

    public void init() {
        logger.info("Start init MQTTClient");
        connectOptions = new MqttConnectOptions();
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
                    properties.getProperty(CLIENT_ID));
            client.setCallback(callback);
            client.connect(connectOptions);
            logger.info("Init MQTTClient successfully");
        } catch (MqttException e) {
            logger.error(e);
        }
    }
}
