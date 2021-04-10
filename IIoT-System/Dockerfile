FROM maven:3.5-jdk-8 AS build  
COPY src /usr/src/app/src  
COPY preinstall /usr/src/app/preinstall  
COPY pom.xml /usr/src/app 
RUN mvn -f /usr/src/app/preinstall/preinstall.xml clean install 
RUN mvn -f /usr/src/app/pom.xml clean package

FROM openjdk:8
COPY --from=build /usr/src/app/target/mainproj.data-1.0-SNAPSHOT.jar /usr/app/mainproj.data-1.0-SNAPSHOT.jar
COPY --from=build /usr/src/app/src/main/resources /usr/app/src/main/resources