run “./vm”
it will say vm failed to start because my script doesn’t wait long enough on the first start.
it will download the images and install the rpm when you first run it.
once that’s done.
you run “./vm” again, it will give you a ssh prompt.
you can run “./vm” again in another terminal and it will give you another ssh prompt to the same image container.
"./vm stop" to stop the container
"./vm destroy" will destory the image and the next time you run “./vm” again, it will create the image from scratch.
It mounts /Volumes/Projects/core-cpp to /usr/local/core-cpp  
you might need to change that in the docker-compose.yml file if you want to change the mount point
