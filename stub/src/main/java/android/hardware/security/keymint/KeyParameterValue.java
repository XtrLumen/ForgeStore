package android.hardware.security.keymint;

import android.os.Parcel;
import android.os.Parcelable;

public class KeyParameterValue implements Parcelable {
    public int algorithm;
    public int integer;
    public long longInteger;
    public byte[] blob;
    public boolean boolValue;
    public long dateTime;
    public int ecCurve;
    public int origin;
    public int blockMode;
    public int paddingMode;
    public int keyPurpose;
    public int digest;

    public static final Parcelable.Creator<KeyParameterValue> CREATOR =
            new Parcelable.Creator<>() {
                public KeyParameterValue createFromParcel(Parcel in) {
                    KeyParameterValue v = new KeyParameterValue();
                    int disc = in.readInt();
                    switch (disc) {
                        case 0: v.algorithm = in.readInt(); break;
                        case 1: v.integer = in.readInt(); break;
                        case 2: v.longInteger = in.readLong(); break;
                        case 3: v.blob = in.createByteArray(); break;
                        case 4: v.boolValue = in.readInt() != 0; break;
                        case 5: v.dateTime = in.readLong(); break;
                        case 6: v.ecCurve = in.readInt(); break;
                        case 7: v.origin = in.readInt(); break;
                        case 8: v.blockMode = in.readInt(); break;
                        case 9: v.paddingMode = in.readInt(); break;
                        case 10: v.keyPurpose = in.readInt(); break;
                        case 11: v.digest = in.readInt(); break;
                    }
                    return v;
                }
                public KeyParameterValue[] newArray(int size) {
                    return new KeyParameterValue[size];
                }
            };

    @Override
    public int describeContents() { return 0; }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        throw new UnsupportedOperationException("STUB!");
    }
}
