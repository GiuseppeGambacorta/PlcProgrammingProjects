plugins {
    kotlin("jvm") version "2.4.20"
}

group = "org.example"
version = "1.0-SNAPSHOT"

repositories {
    mavenCentral()
}

dependencies {
    // Apache PLC4X API e Driver Modbus
    implementation("org.apache.plc4x:plc4j-api:0.13.1")
    implementation("org.apache.plc4x:plc4j-driver-modbus:0.13.1")

    // Logger SLF4J (richiesto da PLC4X)
    implementation("org.slf4j:slf4j-simple:2.0.17")

    testImplementation(kotlin("test"))
}

kotlin {
    jvmToolchain(26)
}

tasks.test {
    useJUnitPlatform()
}
