#NAME: William Chong
#EMAIL: williamchong256@gmail.com
#ID: 205114665

#!/bin/bash

#script to run lab0 against several test cases and check if expected behavior

#create a file for testing input/output
echo "hello this is a test text file" > input.txt

#check if correct, normal arguments generate expected exit code of 0
`./lab0 --input input.txt --output output.txt`
if [ $? -eq 0 ]
then
	echo "Testing regular arguments, expect an exit code of 0... Program exited correctly!"
else
	echo "Testing regular arguments, expect an exit code of 0... Program exited with incorrect exit code."
fi


#check if the input and output files match
`cmp input.txt output.txt`
if [ $? -eq 0 ]
then
	echo "Checking if input text and output text matches... Program copied text correctly!"
else
	echo "Checking if input text and output text matches... Error: text not copied correctly."
fi


#check if handles unrecognized argument correctly, should generate exit code of 1
`./lab0 --bogus --output output.txt`
if [ $? -eq 1 ]
then
	echo "Testing unrecognized argument, expect an exit code of 1... Program exited as expected after detecting unrecognized argument."
else
	echo "Testing unrecognized argument, expect an exit code of 1... Program exited with incorrect exit code."
fi


#check if handles error opening input file correctly should exit with 2
`./lab0 --input somewrongfilethatdoesnotexistforsure --output output.txt`
if [ $? -eq 2 ]
then
	echo "Testing input file opening error, expect an exit code of 2... Program exited as expected!"
else
	echo "Testing input file opening error, expect an exit code of 2... Program exited with incorrect exit code."
fi


#check if handles error writing to output file correctly, expect exit(3)
`./lab0 --input input.txt --output /some/path/that/definitely/hopefully/does/not/exist/h4h4.txt`
if [ $? -eq 3 ]
then
	echo "Testing output file error, expect an exit code of 3... Program exited as expected!"
else
	echo "Testing output file error, expect an exit code of 3... Program exited with incorrect exit code."
fi


#check if catches segfault, expect exit(4)
`./lab0 --input input.txt --output output.txt --segfault --catch`
if [ $? -eq 4 ]
then
	echo "Testing segfault and catch option... Program exited as expected!"
else 
	echo "Testing segfault and catch option... Program exited with incorrect exit code."
fi



#clear files
`rm -f *.txt`
