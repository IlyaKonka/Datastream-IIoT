package vibration;

import flink.Main;
import org.apache.flink.api.java.tuple.Tuple5;
import org.apache.flink.configuration.Configuration;
import org.apache.flink.streaming.api.functions.sink.RichSinkFunction;
import org.apache.log4j.Logger;

public class VibrationAnomalySink extends RichSinkFunction<Tuple5<String, Double, Double, Double, Long>> {
    private static Logger logger = Logger.getLogger(VibrationAnomalySink.class);

    @Override
    public void open(Configuration parameters) throws Exception {
        super.open(parameters);
    }

    @Override
    public void close() throws Exception {
        super.close();
    }

    @Override
    public void invoke(Tuple5<String, Double, Double, Double, Long> dataPoint, Context context) {
//        logger.info(String.format("Anomaly: %s | X: %.4f | Y: %.4f | Z: %.4f | Time: %s",
//                dataPoint.f0, dataPoint.f1, dataPoint.f2, dataPoint.f3,
//                Main.sdf.format(new Date(dataPoint.f4))));
        Main.motorController.checkAnomaly(dataPoint.f1, dataPoint.f2, dataPoint.f3);
    }

}
