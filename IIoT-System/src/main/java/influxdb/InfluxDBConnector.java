package influxdb;

import lombok.Getter;
import org.apache.log4j.Logger;
import org.influxdb.BatchOptions;
import org.influxdb.InfluxDB;
import org.influxdb.InfluxDBFactory;
import org.influxdb.dto.Query;

import java.io.Serializable;
import java.util.Properties;

@Getter
public class InfluxDBConnector implements Serializable {
    private static final String URL = "URL";
    private static final String PORT = "PORT";
    private static final String USERNAME = "USERNAME";
    private static final String PASSWORD = "PASSWORD";
    private static final String DB_NAME = "DB_NAME";
    private static final long serialVersionUID = -3189811537194196139L;

    private static Logger logger = Logger.getLogger(InfluxDBConnector.class);
    private transient InfluxDB influxDB;

    private Properties properties;

    public InfluxDBConnector(Properties properties) {
        this.properties = properties;
    }

    public void connect() throws Exception {
        influxDB = InfluxDBFactory.connect(properties.getProperty(URL) + ":" + properties.getProperty(PORT),
                properties.getProperty(USERNAME), properties.getProperty(PASSWORD));
        if (!influxDB.databaseExists(properties.getProperty(DB_NAME))) {
            influxDB.query(new Query("CREATE DATABASE " + properties.getProperty(DB_NAME)));
        }
        influxDB.setDatabase(properties.getProperty(DB_NAME));
        influxDB.enableBatch(BatchOptions.DEFAULTS);
        influxDB.setLogLevel(InfluxDB.LogLevel.NONE);
    }

}
