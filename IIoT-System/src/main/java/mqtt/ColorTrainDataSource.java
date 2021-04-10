package mqtt;

import flink.Main;
import org.apache.flink.api.java.tuple.Tuple4;
import org.apache.flink.streaming.api.functions.source.RichSourceFunction;
import org.apache.log4j.Logger;

import java.nio.charset.StandardCharsets;
import java.util.Date;

public class ColorTrainDataSource extends RichSourceFunction<Tuple4<String, String, String, Long>> {
    private static Logger logger = Logger.getLogger(ColorTrainDataSource.class);

    // ------ Runtime fields
    private transient volatile boolean running;
    private transient Object waitLock;

    private String mqttTopic;

    public ColorTrainDataSource(String topic) {
        this.mqttTopic = topic;
    }

    @Override
    public void run(SourceContext<Tuple4<String, String, String, Long>> ctx) throws Exception {
        Main.mqttClient.getClient().subscribe(mqttTopic, (topic, message) -> {
            String msg = new String(message.getPayload(), StandardCharsets.UTF_8);
            String quality = "none";
            if (msg.startsWith("0")) {
                quality = "good";
            } else if (msg.startsWith("1")) {
                quality = "defect1";
            } else if (msg.startsWith("2")) {
                quality = "defect2";
            }
            msg = msg.substring(2); //delete quality info
            long currentTimeMs = System.currentTimeMillis();
            logger.info(String.format("Measurement: %s Quality: %s\nData: %s\nTime: %s",
                    mqttTopic, quality, msg,
                    Main.sdf.format(new Date(currentTimeMs))));
            ctx.collect(new Tuple4<>(mqttTopic, quality, msg, currentTimeMs));
            //ctx.emitWatermark(new Watermark(currentTimeMs));
        });
        running = true;
        waitLock = new Object();
        while (running) {
            synchronized (waitLock) {
                waitLock.wait(100L);
            }
        }
    }

    @Override
    public void cancel() {
        running = false;
    }

}
