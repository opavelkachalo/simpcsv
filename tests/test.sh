#/bin/sh

cd $(dirname $0)

flag="f"
if [ $# -ge 1 ]; then
	flag=$1
fi

n_succ=0
n_fail=0
for csv in $(ls in/); do
	../simpcsv in/$csv > tmp
	if ! diff tmp out/$csv.out &>/dev/null; then
		n_fail=$((n_fail + 1))
		if [ $flag = "f" -o $flag = "sf" ]; then
			echo ========= Test failed =========
			echo [INPUT] \(in/$csv\):
			cat in/$csv
			echo -e "\n[EXPECTED]" \(out/$csv.out\):
			cat out/$csv.out
			echo -e "\n[GOT]:"
			cat tmp
		fi
	else
		n_succ=$((n_succ + 1))
		if [ $flag = "s" -o $flag = "sf" ]; then
			echo ========= Test succeeded =========
			echo [INPUT] \(in/$csv\):
			cat in/$csv
			echo -e "\n[GOT]:"
			cat tmp
		fi
	fi
done

echo ========= Results =========
echo passed $n_succ tests
echo failed $n_fail tests

if [ -e tmp ]; then
	rm tmp
fi

cd - &>/dev/null
