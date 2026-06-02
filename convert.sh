#!/system/bin/sh
# convert.sh — keybox.xml to DER binaries
# Usage: sh convert.sh <keybox.xml> [output_dir]

if [ $# -lt 1 ]; then
    echo "Usage: sh convert.sh <keybox.xml> [output_dir]"
    exit 1
fi

KEYBOX="$1"
OUTDIR="${2:-${MODDIR:-.}/keys}"
mkdir -p "$OUTDIR"
[ ! -f "$KEYBOX" ] && { echo "keybox not found: $KEYBOX"; exit 1; }

echo "Converting $KEYBOX -> $OUTDIR"

write_be32() {
    local v=$1
    printf "\\$(printf '%03o' $((v >> 24 & 0xFF)))"
    printf "\\$(printf '%03o' $((v >> 16 & 0xFF)))"
    printf "\\$(printf '%03o' $((v >> 8 & 0xFF)))"
    printf "\\$(printf '%03o' $((v & 0xFF)))"
}

awk '/BEGIN EC PRIVATE KEY/{f=1; next} /END EC PRIVATE KEY/{f=0} f' "$KEYBOX" \
    | tr -d '\n' | base64 -d > "$OUTDIR/privkey_ec.der" 2>/dev/null
[ -s "$OUTDIR/privkey_ec.der" ] || rm -f "$OUTDIR/privkey_ec.der"

awk '/BEGIN RSA PRIVATE KEY/{f=1; next} /END RSA PRIVATE KEY/{f=0} f' "$KEYBOX" \
    | tr -d '\n' | base64 -d > "$OUTDIR/privkey_rsa.der" 2>/dev/null
[ -s "$OUTDIR/privkey_rsa.der" ] || rm -f "$OUTDIR/privkey_rsa.der"

awk '
/BEGIN CERTIFICATE/{f=1; buf=""; next}
/END CERTIFICATE/{f=0; print buf; next}
f{buf = buf $0}
' "$KEYBOX" > "$OUTDIR/cert_pems.tmp"

NUM=0
while IFS= read -r line; do
    [ -z "$line" ] && continue
    echo "$line" | base64 -d > "$OUTDIR/cert_$NUM.der" 2>/dev/null
    NUM=$((NUM + 1))
done < "$OUTDIR/cert_pems.tmp"
rm -f "$OUTDIR/cert_pems.tmp"

write_be32 "$NUM" > "$OUTDIR/cert_chain.der"
for i in $(seq 0 $((NUM - 1))); do
    f="$OUTDIR/cert_$i.der"
    [ -f "$f" ] && write_be32 "$(wc -c < "$f")" >> "$OUTDIR/cert_chain.der" && cat "$f" >> "$OUTDIR/cert_chain.der"
    rm -f "$f"
done

echo "Done: $NUM certificates"
