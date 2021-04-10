package influxdb;


import flink.Main;
import org.apache.flink.api.java.tuple.Tuple4;
import org.apache.flink.configuration.Configuration;
import org.apache.flink.streaming.api.functions.sink.RichSinkFunction;
import org.apache.log4j.Logger;
import org.influxdb.dto.Point;

import java.util.Date;
import java.util.concurrent.TimeUnit;

public class ColorDataSink extends RichSinkFunction<Tuple4<String, String, String, Long>> {

    private static Logger logger = Logger.getLogger(ColorDataSink.class);
    public static final String QUALITY_FIELD = "quality";
    public static final String DATA_FIELD = "datapoints";

    @Override
    public void open(Configuration parameters) throws Exception {
        super.open(parameters);
    }

    @Override
    public void close() throws Exception {
        super.close();
    }

    @Override
    public void invoke(Tuple4<String, String, String, Long> dataPoint, Context context) {
        logger.info(String.format("Measurement: %s Quality: %s\nData: %s\nTime: %s",
                dataPoint.f0, dataPoint.f1, dataPoint.f2,
                Main.sdf.format(new Date(dataPoint.f3))));
        Point.Builder builder = Point.measurement(dataPoint.f0)
                .time(dataPoint.f3, TimeUnit.MILLISECONDS)
                .addField(QUALITY_FIELD, dataPoint.f1)
                .addField(DATA_FIELD, dataPoint.f2);

        Point point = builder.build();
        Main.influxDBConnector.getInfluxDB().write(point);
    }
}
