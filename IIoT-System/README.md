# IIoT-System
## Getting Started
Install docker https://docs.docker.com/get-docker/
## How to use
### Startup
At first create docker network
```
docker network create --subnet 172.30.0.0/24 flinkprojnetwork
```
Then just run **docker-compose.yml** from command line
```
docker-compose up
```
### Demonstration
In the log output of **flink_proj_javaAPI** container you can see the fake data from color/vibration sensor simulators.
There is vibration data graph in [Grafana](http://localhost:3000) that is also available for simulated vibrations.
**MapReduce** checks periodically the number of color datasets in **Hadoop File System** and if the defined number (**150** sets by default) of color datasets was reached
(about **10 minutes** after start of the program), you can see the predictions of **k-nearest neighbors algorithm** for new generated fake color datasets. 


### Web UIs
- [Grafana](http://localhost:3000) (credentials _admin:admin_)
- [Chronograf](http://localhost:8888) (credentials _admin:admin_)
- [Hadoop Web UI](http://localhost:9870)

### Remove HDFS Cache
Run command
```
docker volume rm iiotsystem_hadoop_namenode iiotsystem_hadoop_datanode1 iiotsystem_hadoop_datanode2
```

## Built With
- [Apache Flink](https://flink.apache.org)
- [Grafana](https://grafana.com)
- [InfluxDB](https://www.influxdata.com/)
- [Chronograf](https://docs.influxdata.com/chronograf/v1.8/)
- [Mosquitto](https://mosquitto.org/)
- [Paho](https://www.eclipse.org/paho/)
- [Maven](https://maven.apache.org/)
- [Hadoop](https://hadoop.apache.org/)
- [JavaML](http://java-ml.sourceforge.net/)
- [docker-compose](https://docs.docker.com/compose/)

## Inspired by
Project structure and some implementations:
- [Jamie Grier](https://github.com/jgrier)
- [Maximilian Bode](https://github.com/mbode)
- [Anton Rudacov](https://github.com/antonrud)
- [Prashant Khanal](https://github.com/pkhanal)
- [Big Data Europe](https://github.com/big-data-europe/docker-hadoop)
