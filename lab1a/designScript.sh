#! /bin/sh

# UCLA CS 111 Lab 1 - Test that valid commands are processed correctly.
a >> b      	#append a to end of b
a -b >> b	#add a to beginning of b
a -e >> b	#append a to end of b

<&2		#[default to 0]<&digit
<&word		#[default to 0]<&word
2<&5		#[n]<&digit
2<&word		#[n]<&word

>&2		#[default to 0]>&digit
>&word		#[default to 0]>&word
2>&5		#[n]>&digit
2>&word		#[n]>&word

<>word		#[default to 0]digit
3<>word		#[n]<>word
