package influxdb;


import flink.Main;
import org.apache.flink.api.java.tuple.Tuple5;
import org.apache.flink.configuration.Configuration;
import org.apache.flink.streaming.api.functions.sink.RichSinkFunction;
import org.apache.log4j.Logger;
import org.influxdb.dto.Point;

import java.util.concurrent.TimeUnit;

public class VibrationSink extends RichSinkFunction<Tuple5<String, Double, Double, Double, Long>> {

    private static Logger logger = Logger.getLogger(VibrationSink.class);

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
//        logger.info(String.format("Measurement: %s | X: %.4f | Y: %.4f | Z: %.4f | Time: %s",
//                dataPoint.f0, dataPoint.f1, dataPoint.f2, dataPoint.f3,
//                Main.sdf.format(new Date(dataPoint.f4))));
        Point.Builder builder = Point.measurement(dataPoint.f0)
                .time(dataPoint.f4, TimeUnit.MILLISECONDS)
                .addField("x", dataPoint.f1)
                .addField("y", dataPoint.f2)
                .addField("z", dataPoint.f3);

        Point point = builder.build();
        Main.influxDBConnector.getInfluxDB().write(point);
    }
}
