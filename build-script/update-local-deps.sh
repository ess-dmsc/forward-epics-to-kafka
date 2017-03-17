if [ -d "streaming-data-types" ]
then

if [ ! -d "streaming-data-types/.git" ]
then
echo "THIS LOOKS LIKE A BROKEN BUILD DIRECTORY.  SHOULD WIPE EVERYTHING."
else
cd streaming-data-types && git pull
fi

else
git clone https://github.com/ess-dmsc/streaming-data-types.git

fi
