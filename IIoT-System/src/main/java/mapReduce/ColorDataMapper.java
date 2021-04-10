package mapReduce;

import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.log4j.Logger;

import java.io.IOException;

public class ColorDataMapper extends Mapper<LongWritable, Text, Text, Text> {
    private static Logger logger = Logger.getLogger(ColorDataMapper.class);

    @Override
    protected void map(LongWritable key, Text value, Context context) throws IOException, InterruptedException {
        String[] elems = value.toString().split(";");
        try {
            context.write(new Text(elems[0].substring(0, elems[0].indexOf("T"))), new Text(elems[1]));
            //key=day value=quality
        }catch (Exception e){
            logger.error(e);
            logger.error("mapping problem");
        }
    }
}
