#!/bin/bash

rm file*
make testraid
make testssd
filesize=100
noupdates=150
for (( noupdates=100; noupdates<=2000; noupdates+=100 ))
do
	outputraid=`./testraid $filesize $noupdates | grep 'output final reads'`
	outputssd=`./testssd $filesize $noupdates | grep 'output final reads'`
	#echo $outputraid
	#echo $outputssd
	raid=${outputraid#o*-}
	x=${outputssd##o*-}
	y=${outputssd%$x}
	z=${y%o*-}
	seqssd=${z#o*-}
	randssd=$x
	echo $noupdates,$((noupdates*43/10)),$((randssd*19/10))
	rm file*
done
