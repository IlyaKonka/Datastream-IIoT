package mqtt;

import flink.Main;
import org.apache.flink.api.java.tuple.Tuple5;
import org.apache.flink.streaming.api.functions.source.RichSourceFunction;
import org.apache.log4j.Logger;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashMap;

import static flink.Main.isSimulation;

public class VibrationSource extends RichSourceFunction<Tuple5<String, Double, Double, Double, Long>> {
    private static Logger logger = Logger.getLogger(VibrationSource.class);

    // ------ Runtime fields
    private transient volatile boolean running;
    private transient Object waitLock;

    private String mqttTopic;

    public static final double gConst = 9.813; //Berlin
    public static final int LIS3DH_LSB16_TO_KILO_LSB10 = 64000;
    public static final int LIS3DH_LSB_VAL = 4;

    public VibrationSource(String topic) {
        this.mqttTopic = topic;
    }

    @Override
    public void run(SourceContext<Tuple5<String, Double, Double, Double, Long>> ctx) throws Exception {
        Main.mqttClient.getClient().subscribe(mqttTopic, (topic, message) -> {
            if (isSimulation) {
                String msg = new String(message.getPayload(), StandardCharsets.UTF_8);
                String[] accel = msg.split(";");
                long currentTimeMs = System.currentTimeMillis();
                ctx.collect(new Tuple5<>(mqttTopic, Double.parseDouble(accel[0]), Double.parseDouble(accel[1]),
                        Double.parseDouble(accel[2]), currentTimeMs));
            } else {
                HashMap<String, Double> accel = parseMsg(message);
//            logger.info(String.format("Measurement: %s | X: %.4f | Y: %.4f | Z: %.4f | Time: %s",
//                    mqttTopic, accel.get("x"), accel.get("y"), accel.get("z"),
//                    Main.sdf.format(new Date(currentTimeMs))));
                long currentTimeMs = System.currentTimeMillis();
                ctx.collect(new Tuple5<>(mqttTopic, accel.get("x"), accel.get("y"), accel.get("z"), currentTimeMs));
                //ctx.emitWatermark(new Watermark(currentTimeMs));
            }
        });
        running = true;
        waitLock = new Object();
        while (running) {
            synchronized (waitLock) {
                waitLock.wait(100L);
            }
        }
    }

    private HashMap<String, Double> parseMsg(MqttMessage message) {
        String msgString = Arrays.toString(convert(message.getPayload())).replaceAll(" ", "")
                .replace("[", "").replace("]", "");
        HashMap<String, Double> accel = new HashMap<>();
        try {
            String[] data = msgString.split(",");
            int counter = 0;
            for (int i = 22; i < data.length; i = i + 3) {
                try {
                    double x_a = parseGravity(Double.parseDouble(data[i]));
                    double y_a = parseGravity(Double.parseDouble(data[i + 1]));
                    double z_a = colibrateForVisualisation(parseGravity(Double.parseDouble(data[i + 2])));
                    double x_sum = accel.getOrDefault("x", 0.0);
                    double y_sum = accel.getOrDefault("y", 0.0);
                    double z_sum = accel.getOrDefault("z", 0.0);
                    accel.put("x", x_sum + x_a);
                    accel.put("y", y_sum + y_a);
                    accel.put("z", z_sum + z_a);

                    //logger.info(String.format("SET #%d: %f | %f | %f", counter, x_a, y_a, z_a));
                    counter++;
                } catch (Exception e) {
                    logger.error("Incorrect vibration data. No info about one axis acceleration");
                }
            }
            accel.put("x", accel.get("x") / ((double) counter));
            accel.put("y", accel.get("y") / ((double) counter));
            accel.put("z", accel.get("z") / ((double) counter));
            return accel;

        } catch (Exception e) {
            logger.error("Problem during acceleration parse");
        }

        return null;
    }

    private double colibrateForVisualisation(double a) {
        double coeff = 10.0;
        return a - coeff;
    }

    private short[] convert(byte[] byteArr) {
        int[] unsignedArr = new int[byteArr.length];
        for (int i = 0; i < byteArr.length; i++) {
            unsignedArr[i] = byteArr[i] & 0xFF;
        }

        short[] arrVib = new short[22 + 15]; // 15 is count of vibration values
        arrVib[0] = (short) (unsignedArr[0]); // location
        arrVib[1] = (short) (unsignedArr[1]); // sensor_id

        arrVib[2] = (short) (unsignedArr[2]); // motor_state[0]
        arrVib[3] = (short) (unsignedArr[3]); // motor_state[1]
        arrVib[4] = (short) (unsignedArr[4]); // motor_state[2]
        arrVib[5] = (short) (unsignedArr[5]); // motor_state[3]
        arrVib[6] = (short) (unsignedArr[6]); // motor_state[4]

        arrVib[7] = (short) (unsignedArr[7]); // motor_direction[0]
        arrVib[8] = (short) (unsignedArr[8]); // motor_direction[1]
        arrVib[9] = (short) (unsignedArr[9]); // motor_direction[2]
        arrVib[10] = (short) (unsignedArr[10]); // motor_direction[3]
        arrVib[11] = (short) (unsignedArr[11]); // motor_direction[4]

        arrVib[12] = (short) (unsignedArr[12]); // motor_speed[0]
        arrVib[13] = (short) (unsignedArr[13]); // motor_speed[1]
        arrVib[14] = (short) (unsignedArr[14]); // motor_speed[2]
        arrVib[15] = (short) (unsignedArr[15]); // motor_speed[3]
        arrVib[16] = (short) (unsignedArr[16]); // motor_speed[4]

        arrVib[17] = (short) (unsignedArr[17] - 48); // pos[0]
        arrVib[18] = (short) (unsignedArr[18] - 48); // pos[1]
        arrVib[19] = (short) (unsignedArr[19] - 48); // pos[2]
        arrVib[20] = (short) (unsignedArr[20] - 48); // pos[3]

        if (new Short((short) (unsignedArr[21])).compareTo(new Short((short) 45)) == 0) // "-"
            arrVib[21] = 0; // pos[4]
        else
            arrVib[21] = (short) (unsignedArr[21] - 48); // pos[4]

        // position is 0 if no material on the module
        int j = 22;
        for (int i = 22; i < arrVib.length; i++) // 15 vib values
        {
            arrVib[i] = (short) ((unsignedArr[j] << 8) & 0b00000000000000001111111100000000);
            arrVib[i] += unsignedArr[j + 1];
            j += 2;
        }

        return arrVib;
    }

    private double parseGravity(double acceleration) {
        return gConst * ((double) LIS3DH_LSB_VAL) * (acceleration / ((double) LIS3DH_LSB16_TO_KILO_LSB10));
    }

    @Override
    public void cancel() {
        running = false;
    }

}
