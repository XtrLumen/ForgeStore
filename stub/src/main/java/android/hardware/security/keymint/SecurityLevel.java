package android.hardware.security.keymint;

public @interface SecurityLevel {
    int SOFTWARE = 0;
    int TRUSTED_ENVIRONMENT = 1;
    int STRONGBOX = 2;
}
