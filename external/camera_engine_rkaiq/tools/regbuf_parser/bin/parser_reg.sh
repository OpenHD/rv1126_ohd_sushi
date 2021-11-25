#!/bin/bash

if [ -n "$1" ];then
	IN_VIDEO_TYPE="$1"
else
	echo "Please enter the video type!" 
	IN_VIDEO_TYPE="h264"
fi

if [ -n "$2" ];then
	IN_VIDEO="$2"
	echo "IN_VIDEO: $IN_VIDEO"
	RESULT=${IN_VIDEO%.*}
	echo "RESULT: $RESULT"
	mkdir $RESULT
	OUT_VIDEO=${IN_VIDEO%.*}".h264"
	echo "OUT_VIDEO: $OUT_VIDEO"
fi

if [ IN_VIDEO_TYPE != "h264" ];then
	ffmpeg -i $IN_VIDEO -vcodec copy $RESULT/$OUT_VIDEO > /dev/null 2>&1
    cd $RESULT
fi

if [ -n "$3" ];then
	FRAME_START_ID=$3
	echo "FRAME_START_ID: $FRAME_START_ID"
fi

if [ -n "$4" ];then
	FRAME_END_ID=$4
	echo "FRAME_END_ID: $FRAME_END_ID"
fi

while(( $FRAME_START_ID<=$FRAME_END_ID ))
do
	../sei_to_ispp_reg $OUT_VIDEO $FRAME_START_ID > /dev/null 2>&1

	PARSER_REG_FILE=$FRAME_START_ID"_ispp.reg"
	mv ispp.reg $PARSER_REG_FILE
	# echo "PARSER_REG_FILE: $PARSER_REG_FILE"
	chmod 777 $PARSER_REG_FILE

	let "FRAME_ID=$FRAME_START_ID-1"
	echo -e "\nframe$FRAME_ID: " | tee -a $RESULT".log"
    ../regbuf_parser $PARSER_REG_FILE | tee "frame"$FRAME_ID".reg" | grep -E "ae luma avg|frame_id|ae luma exposure|_ts" | tee -a $RESULT".log"
    # # ../regbuf_parser $PARSER_REG_FILE | tee "frame"$FRAME_ID".reg" | grep "frame_id" 

	let "FRAME_START_ID++"
done

rm *ispp.reg
cd -
