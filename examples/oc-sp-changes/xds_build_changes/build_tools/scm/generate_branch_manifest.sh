#!/bin/bash
repos=`find * -name .git`
for repo in $repos;
do
    origin=`grep "url = " $repo/config | awk -F'=' '{ gsub(/ /, "", $2); print $2 }'`
    repo_dir=$(dirname $repo)
    branch=`(cd $repo_dir; git branch | grep \* | awk '{ print $2 }')` 
    echo -e "$repo_dir\t$branch\t$origin" >> branch.manifest.temp
done

cat branch.manifest.temp | column -t > branch.manifest
rm branch.manifest.temp
