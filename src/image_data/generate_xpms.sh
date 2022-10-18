#!/bin/bash

backgrounds="off on"
mem_dimensions=( 22 44 10 10)
swap_dimensions=(11 22 21 21)
number_format="%02d"

###
# exit handler, removes the remporary working directory
###
function cleanup() {
	if [ -n "${tmp}" ]; then
		rm -rf ${tmp}
	fi
}

###
# generate file name for output (png) based on arguments
#
# @param tmp	temporary directory to prepend to file name
# @param type	type value (mem/swap)
# @param lit	lit setting ("on/off/color" etc)
# @param i		number of the file
# @param prefix	prefix of file name
# @param ext	extension of the file (defaults to 'png')
###
function file_name() {
	local tmp=${1}
	local type=${2}
	local lit=${3}
	local i=${4}
	local prefix=${5:-""}
	local ext=${6:-png}

	printf "%s/%s%s_%s_${number_format}.%s" "${tmp}" \
		"${prefix}" "${type}" "${lit}" ${i} "${ext}"
}

###
# generate output pngs for \c file
#
# @param tmp	temporary directory to prepend to file name
# @param svg	input svg to generate pngs for
# @param bgs	lit setting ("on off color" etc)
# @param types	types of files ("mem swap")
# @param count	number of section (resolution) of the mem/swap segments
###
function generate_pngs() {
	local tmp=${1}
	local svg=${2}
	local bgs=(${3})
	local types=(${4})
	local count=(${5})
	local items=(seq 1 ${count})

	for b in $(seq 0 $((${#bgs[*]}-1))); do

		back=${bgs[$b]}
		back_filter=$(echo " ${bgs[*]}" | \
				sed -e "s/ ${back}//; s/ / background_/g; s/^ //; s/ \+/|/")

		for t in $(seq 0 $((${#types[*]}-1))); do

			type=${types[$t]}
			type_filter=$(echo " ${types[*]}" | \
					sed -e "s/ ${type}//; s/ /tile_/g; s/^ //; s/ \+/|/")

			filter="${back_filter}|${type_filter}"
			for i in $(seq ${count} -1 1); do
				out=$(file_name "${tmp}" "${types[$t]}" "${bgs[$b]}" ${i} "parts_")
				grep -vE "${filter}" "${svg}" | \
					inkscape -p -C -d 96 -o ${out} 2>/dev/null
				echo -ne "generated ${out}                            \r"
				filter="${filter}|tile_${type}_"$(printf "${number_format}" $i)
			done
		done
	done

	printf "png generation completed                                    \n"
}

###
# crop relevant parts from the pngs created by generate_pngs
# Note: once half the images were generated, \c xoff is incremented by
#       width to catch the other side of the circle (i.e. the input
#       is supposed to be symmetrical
#
# @param tmp	temporary directory to prepend to file name
# @param type	type value (mem/swap)
# @param lit	lit setting ("on/off/color" etc)
# @param count	number of images to crop
# @param width	width of the cropped image
# @param height	height of the cropped image
# @param xoff	x-offset in the image to be cropped
# @param yoff	y-offset in the image to be cropped
###
function crop_pngs() {
	local tmp=${1}
	local type=${2}
	local lit=${3}
	local count=${4}
	local width=${5}
	local height=${6}
	local xoff=${7}
	local yoff=${8}

	seq 1 ${count} | while read i; do
		in=$(file_name "${tmp}" "${type}" "${lit}" ${i} "parts_")
		out=$(file_name "${tmp}" "${type}" "${lit}" ${i})

		if [ $i -gt $((${count}/2)) ]; then
			factor=0
		else
			factor=1
		fi

		convert "${in}" -crop \
			${width}x${height}+$((${xoff}+${factor}*${width}))+${yoff} "${out}"
	done

	echo "cropped ${count} ${type}_${lit} pngs"
}

###
# add a vertical line of transparent pixels to the xpm; otherwise
# DAMakeShapedPixmapFromData can't work with pixmap properly;
# this is a merely a hack as it seems libdockap cannot deal with
# pixmaps that don't have transparent parts;
#
# @param file	file name to check for (and add) transparent pixels
###
function transparency_fixup() {
	local file=${1}

	grep -i ' c none"' "${file}" >/dev/null || (
		identify -format "%w %h\n" "${file}" | while read w h; do
			w=$((${w}+1))
			mogrify -background none -extent ${w}x${h} "${file}"
			echo "added transparent pixels to ${file}"
		done
	)
}

###
# crop relevant parts from the pngs created by generate_pngs
# Note: once half the images were generated, \c xoff is incremented by
#       width to catch the other side of the circle (i.e. the input
#       is supposed to be symmetrical
#
# @param tmp	temporary directory to prepend to file name
# @param bgs	lit setting ("on off color" etc)
# @param types	type value ("mem swap" etc)
# @param count	number of section (resolution) of the mem/swap segments
###
function montage_pngs() {
	local tmp=${1}
	local bgs=(${2})
	local types=(${3})
	local count=${4}

	for t in $(echo ${types[*]}); do
		unset files
		for b in $(echo ${bgs[*]}); do
			if [ "mem" = "${t}" ]; then
				crop_pngs "${tmp}" "${t}" "${b}" ${count} ${mem_dimensions[*]}
			elif [ "swap" = "${t}" ]; then
				crop_pngs "${tmp}" "${t}" "${b}" ${count} ${swap_dimensions[*]}
			fi

			out="${tmp}/${t}_${b}.png"
			convert "${tmp}/${t}_${b}_[0-9]*.png" +append "${out}"
			files="${files} ${out}"
		done

		convert ${files} -append "parts_${t}.xpm"
		transparency_fixup "parts_${t}.xpm"
		echo "created parts_${t}.xpm"
	done
}

###
#
# @param tmp	temporary directory to prepend to file name
# @param svg	input svg to generate pngs for
# @param bgs	lit setting ("on off color" etc)
###
function generate_backdrops() {
	local tmp=${1}
	local svg=(${2})
	local bgs=(${3})

	for b in $(echo ${bgs[*]}); do

		filter=$(echo " ${bgs[*]}" | \
				sed -e "s/ ${b}//; s/ / background_/g; s/^ //; s/ \+/|/")"|tile_"
		out="${tmp}/backdrop_${b}.png"
		grep -vE "${filter}" "${svg}" | inkscape -p -C -d 96 -o ${out} 2>/dev/null
		xpm="$(basename ${out/.png/.xpm})"
		convert "${out}" "${xpm}"
		echo "generated ${xpm}"
	done
}

if [ $# -lt 1 -o ! -r ${1} ]; then
	echo "readable input file required!"
	exit -1
fi

trap cleanup EXIT
tmp=$(mktemp -d)
types="mem swap"

svg=$(mktemp ${tmp}/tmp.XXXXXXXX.svg)
xmllint "${1}" >${svg}

count=$(($(grep "tile_" ${1} | wc -l)/2))
generate_pngs "${tmp}" "${svg}" "${backgrounds}" "${types}" ${count}
montage_pngs "${tmp}" "${backgrounds}" "${types}" ${count}
generate_backdrops "${tmp}" "${svg}" "${backgrounds}"
