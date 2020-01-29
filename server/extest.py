import os.path

if os.path.isfile('semaphorenotif.txt'):
    print ("File exist")
else:
    os.system("echo -n ' ' >| semaphorenotif.txt")
    print("file created")
os.system("sudo rm semaphorenotif.txt")
