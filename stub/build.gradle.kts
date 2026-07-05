plugins {
    id("com.android.library")
}

dependencies {
    compileOnly("androidx.annotation:annotation:1.9.1")
}

android {
    namespace = "org.matrix.stub"
    buildToolsVersion = "36.0.0"
    compileSdk = 36
    defaultConfig {
        minSdk = 29
        targetSdk = 36
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}