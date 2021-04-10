package mapReduce;

import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.log4j.Logger;

import java.io.IOException;

public class ColorDataReducer extends Reducer<Text, Text, Text, Text> {
    private static Logger logger = Logger.getLogger(ColorDataReducer.class);

    @Override
    protected void reduce(Text key, Iterable<Text> values, Context context) throws IOException, InterruptedException {
        QualityStatistic qualityStatistic = new QualityStatistic();
        for (Text val : values) {
            switch (val.toString()) {
                case QualityStatistic.GOOD:
                    qualityStatistic.setGood(qualityStatistic.getGood() + 1);
                    break;
                case QualityStatistic.DEFECT_1:
                    qualityStatistic.setDefect1(qualityStatistic.getDefect1() + 1);
                    break;
                case QualityStatistic.DEFECT_2:
                    qualityStatistic.setDefect2(qualityStatistic.getDefect2() + 1);
                    break;
                default:
                    logger.warn("unknown quality: " + val.toString());
                    break;
            }
        }
        context.write(key, new Text(qualityStatistic.toString()));
    }

}
