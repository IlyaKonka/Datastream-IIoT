package datasimulator;

import org.apache.log4j.Logger;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.concurrent.ThreadLocalRandom;

public class ColorSensorSimulator extends ParentSimulator implements Runnable {
    private static Logger logger = Logger.getLogger(ColorSensorSimulator.class);
    private static final String fakeDataFile = "src/main/resources/fakeColorData.txt";
    private boolean running = false;

    public ColorSensorSimulator(Properties properties) {
        super(properties, logger);
        running = true;
    }

    @Override
    public void run() {
        connect();
        try {
            ArrayList<String> testData = readCSV(
                    new File(fakeDataFile));
            while (running) {
                int randomSet = ThreadLocalRandom.current().nextInt(0, testData.size());
                logger.info("Published fake color sensor data");
                getClient().publish(getProperties().getProperty(ParentSimulator.TOPIC),
                        new MqttMessage(testData.get(randomSet).getBytes(StandardCharsets.UTF_8)));
                Thread.sleep(500);
            }
        } catch (IOException | InterruptedException | MqttException e) {
            logger.error(e.toString());
        }

    }

    public String getFakeDataSet() throws IOException {
        ArrayList<String> testData = readCSV(
                new File(fakeDataFile));
        int randomSet = ThreadLocalRandom.current().nextInt(0, testData.size());
        String randomDataSet = testData.get(randomSet);

        if (randomDataSet.startsWith("0")) {
            randomDataSet = randomDataSet.replaceFirst("0,", "good;");
        } else if (randomDataSet.startsWith("1")) {
            randomDataSet = randomDataSet.replaceFirst("1,", "defect1;");
        } else if (randomDataSet.startsWith("2")) {
            randomDataSet = randomDataSet.replaceFirst("2,", "defect2;");
        }
        randomDataSet = new SimpleDateFormat("yyyy:MM:dd:HH:mm:ss:SSS").format(new Date()) +
                ";" + randomDataSet;

        return randomDataSet;
    }

    public ArrayList<String> readCSV(File file) throws IOException {
        BufferedReader br = new BufferedReader(new FileReader(file));
        String st = null;
        ArrayList<String> result = new ArrayList<>();
        while ((st = br.readLine()) != null) {
            st = st.replaceAll("'", "");
            List<String> numbs = Arrays.asList(st.split(","));
            result.add(String.join(",", numbs.subList(2, numbs.size()))); //without table name and id
        }
        br.close();
        return result;
    }

}
