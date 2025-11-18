#!/bin/bash

# 샘플 데이터 생성 스크립트
# 소규모 테스트용 PART 및 PARTSUPP 데이터 생성

set -e

DATA_DIR="data"
mkdir -p "$DATA_DIR"

echo "Generating sample TPC-H data..."

# PART 테이블 샘플 데이터 생성 (100개 레코드)
PART_FILE="$DATA_DIR/part.tbl"
echo "Creating $PART_FILE..."

cat > "$PART_FILE" << 'EOF'
1|goldenrod lavender spring chocolate lace|Manufacturer#1|Brand#13|PROMO BURNISHED COPPER|7|JUMBO PKG|901.00|ly. slyly ironi
2|blush thistle blue yellow saddle|Manufacturer#1|Brand#13|LARGE BRUSHED BRASS|1|LG CASE|902.00|lar accounts amo
3|spring green yellow purple cornsilk|Manufacturer#4|Brand#42|STANDARD POLISHED BRASS|21|WRAP CASE|903.00|egular deposits hag
4|cornflower chocolate smoke green pink|Manufacturer#3|Brand#34|SMALL PLATED BRASS|14|MED DRUM|904.00|p furiously r
5|forest brown coral puff cream|Manufacturer#3|Brand#32|STANDARD POLISHED TIN|15|SM PKG|905.00|wake carefully
6|bisque cornflower lawn forest magenta|Manufacturer#2|Brand#24|PROMO PLATED STEEL|4|MED BAG|906.00|sual a
7|moccasin green navajo cream dark|Manufacturer#1|Brand#11|SMALL PLATED COPPER|45|SM BAG|907.00|lyly. ex
8|misty lemon chiffon midnight sky|Manufacturer#4|Brand#44|PROMO BURNISHED TIN|41|LG DRUM|908.00|eposi
9|antique violet turquoise frosted grey|Manufacturer#4|Brand#43|SMALL BURNISHED STEEL|12|WRAP CASE|909.00|ironic foxes wake
10|linen pink saddle puff powder|Manufacturer#5|Brand#54|LARGE BURNISHED STEEL|44|LG CASE|910.00|ithely final deposit
11|blush dim floral saddle navajo|Manufacturer#2|Brand#25|STANDARD BURNISHED COPPER|3|WRAP BOX|911.00|ular courts. furiously final requests across the e
12|cornsilk forest gainsboro azure frosted|Manufacturer#3|Brand#33|MEDIUM ANODIZED STEEL|8|JUMBO CASE|912.00|usly pending p
13|blanched tan lace olive coral|Manufacturer#5|Brand#55|MEDIUM BURNISHED NICKEL|1|JUMBO PACK|913.00|nal theodolites
14|burlywood white wheat royal blush|Manufacturer#1|Brand#14|SMALL PLATED NICKEL|16|MED BAG|914.00|l, express dugouts.
15|khaki ivory smoke azure snow|Manufacturer#1|Brand#15|LARGE ANODIZED BRASS|2|LG BOX|915.00|ar foxes sleep
16|deep lemon dark ghost aquamarine|Manufacturer#3|Brand#31|PROMO BURNISHED BRASS|31|JUMBO BOX|916.00|ts wake furiously regular
17|dark salmon navajo gainsboro tan|Manufacturer#4|Brand#44|ECONOMY PLATED STEEL|14|LG PKG|917.00|gular dependencies. furiously bold requests use slyly express pa
18|seashell floral chartreuse peru sienna|Manufacturer#2|Brand#23|ECONOMY BURNISHED COPPER|13|JUMBO DRUM|918.00|ickly regular requests. pending deposits na
19|light dodger tan rosy sky|Manufacturer#5|Brand#52|SMALL BURNISHED NICKEL|11|SM CASE|919.00|odolites. express deposit
20|ghost chiffon metallic misty bisque|Manufacturer#1|Brand#12|LARGE PLATED COPPER|48|LG CASE|920.00|the blithely regular theodolites. quickly regular
EOF

# PARTSUPP 테이블 샘플 데이터 생성 (각 PART당 4개의 SUPPLIER)
PARTSUPP_FILE="$DATA_DIR/partsupp.tbl"
echo "Creating $PARTSUPP_FILE..."

cat > "$PARTSUPP_FILE" << 'EOF'
1|2|3325|771.64|final theodolites
1|252|8076|993.49|ven ideas. quickly even packages
1|502|3956|337.09|ironic accounts are quickly blithely final
1|752|9191|357.84|al, regular ideas serve
2|3|8895|378.49|nic accounts. deposits are alon
2|253|4969|915.27|ptotes haggle furiously
2|503|8539|438.37|blithely bold ideas. furiously
2|753|4791|471.98|ges. slyly ironic accounts wake blithely
3|4|4651|920.92|ep, express deposits. blithely
3|254|7454|598.37|old dependencies wake. quickly regular
3|504|9942|645.40|r packages sleep against the furiou
3|754|3191|920.85|l pinto beans are slyly. quickly
4|5|1339|113.97|al deposits boost. express
4|255|6377|591.18|usual, even foxes integrate
4|505|9653|834.93|ular foxes are. blithely silent excuses
4|755|9069|314.15|ng to the furiously final p
5|6|6377|54.40|ackages. final deposits sleep
5|256|337|709.87|final, even foxes believe slyly
5|506|4651|567.61|ly pending asymptotes. quickly regular
5|756|6253|76.48|s are slyly. ironic, unusual foxes
6|7|4069|642.13|ly regular requests. blithely pend
6|257|337|130.72|inal pinto beans wake carefully. bold
6|507|6377|424.25|egular packages. slyly final courts
6|757|3956|671.86|ully regular foxes. slyly express
7|8|3325|763.98|y regular packages. fluffily ironic
7|258|9069|551.03|ncies. carefully bold packages use
7|508|9653|763.98|according to the slyly final
7|758|7454|288.22|s sleep blithely. furiously regular
8|9|4651|957.34|ckages haggle carefully. pending,
8|259|7454|680.24|ronic theodolites use blithely. slyly
8|509|1339|113.97|ly pending excuses. ironic
8|759|9942|406.61|lar packages. quickly bold theodolites
9|10|6377|357.84|blithely ironic ideas. express
9|260|9191|357.84|packages cajole furiously. regular
9|510|337|470.09|ar accounts. carefully ironic deposits
9|760|8539|113.97|s affix carefully. final, express theodolites
10|11|3325|357.84|ideas boost carefully. carefully
10|261|3956|274.68|y final requests. blithely bold
10|511|6253|509.42|ges. furiously special pinto beans wake
10|761|4791|582.50|s. furiously regular pinto beans sleep
EOF

echo ""
echo "Sample data generated successfully!"
echo "Files created:"
echo "  - $PART_FILE (20 records)"
echo "  - $PARTSUPP_FILE (40 records)"
echo ""
echo "Next steps:"
echo "1. Build the project: make"
echo "2. Convert CSV to blocks:"
echo "   ./dbsys --convert-csv --csv-file $PART_FILE --block-file data/part.dat --table-type PART"
echo "   ./dbsys --convert-csv --csv-file $PARTSUPP_FILE --block-file data/partsupp.dat --table-type PARTSUPP"
echo "3. Run join:"
echo "   ./dbsys --join --outer-table data/part.dat --inner-table data/partsupp.dat \\"
echo "     --outer-type PART --inner-type PARTSUPP --output output/result.dat --buffer-size 10"
