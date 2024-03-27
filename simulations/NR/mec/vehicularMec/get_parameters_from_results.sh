echo "frequency,numerology,rb,rep,runnumber">myparameters.csv
echo "frequency,numerology,rb,rep,runnumber">/home/hakim/jupyter_dir/enhancing_paper/myparameters.csv

for frequency in {"700MHz","2GHz","6GHz"}
do
	for numerology in {"0","1","2","3","4"}
	do
		for rb in {"15","30","60","120","240","480","960","1920"}
		do
			for rep in {0..99}
			do
				runnumber=$(cat results/teleoperated_driving_2356230/$frequency/$numerology/$rb/$rep.sca  | grep runnumber | cut -d" " -f3 | sed -n 1p)
				echo $frequency,$numerology,$rb,$rep,$runnumber >> myparameters.csv
				echo $frequency,$numerology,$rb,$rep,$runnumber >> /home/hakim/jupyter_dir/enhancing_paper/myparameters.csv


			done
		done
	done	
	 
    
done
