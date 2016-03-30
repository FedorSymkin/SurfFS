echo "building..."
make clean all 2>&1 | grep -i --color -A1000 -B1000 'warning\|error'
echo "building finished"

