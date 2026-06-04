package android.hardware.security.keymint;

public @interface KeyPurpose {
    int VERIFY = 0;
    int ENCRYPT = 4;
    int SIGN = 2;
    int DECRYPT = 3;
    int WRAP_KEY = 5;
    int AGREE_KEY = 6;
    int ATTEST_KEY = 7;
}
