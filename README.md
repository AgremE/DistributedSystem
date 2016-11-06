# Distributed System and Algorithm

This is the example of code project which related distributed system and algorithm. They are purely based on programming assignment of the CS443 class at KAIST. 

## Motivation

Within this page, I want to describd a step by step configuration on MPI Lan and other distributed system like hadoop in which i found a eaiser to understand. 

## Installation

#####MPI LAN Set UP Machine:

Firstly, we should select the master node and set the rest as client node. Then, we have to allow master to connect to client freely with ssh as follow:

    1- install openssh-server  
    
          sudo apt-get install openssh-server (Ubuntus) and sudo dnf install openssh-server (Fedora)
          
    2- You can choose to create the same username among all the nodes for easy access or work with default username. 
    Then, we have to generate the key (id_dsa or id_rsa)
    
          ssh-keygen -t dsa or ssh-keygen -t rsa.
          
    I prefer to use id_ras. Then, we after we send all the key to all the client with the following command:
     
          scp id_rsa client:path_to_your_client_.ssh/
        
    We need to authorize the the key for each of the client with the following command:
      
          cat id_ras >> authorized_keys
        
    From thereafter, master can access the client without using any passwork for confirmation.
    
   
Secondly, we should work on the sharing folder among between all the clients and master node.

    create the folder with the same name for sharing amoung the entire system (Assuming folder name is cloud)
    
    1- We need to install NFS Server on the master node nad NFS client on client node:
       
       sudo apt-get install nfs-kernel-server (Master) and sudo apt-get install nfs-common (client)
     
    2- Then, we need to write a additional line of  *(rw,sync,no_root_squash,no_subtree_check) into the file (only master node) 
    
       cat /etc/exports /home/username/cloud *(rw,sync,no_root_squash,no_subtree_check)
       
       with * can be replace by specific address. You can replace it with all the address like:
       cat /etc/exports /home/username/cloud 123.34.25.32(rw,sync,no_root_squash,no_subtree_check)
       cat /etc/exports /home/username/cloud 123.34.23.31(rw,sync,no_root_squash,no_subtree_check)
       
       but if we want to accept every in comming address, we can just left with start.
       
    3- After we alter exports file, we should call the following commandline:
    
       exportfs -a
        
       Then, we need to restart nfs server service:
       
       sudo service nfs-kernel-server restart
        
    4- Then, we need to mount all the client to cloud folder:
    
        sudo mount -t nfs master:/home/username/cloud ~/cloud
       
    
Thirdly, sharing library configuration for MPI.

    
###Hadoop Machine:

## Tests

Describe and show how to run the tests with code examples.

## Contributors

Let people know how they can dive into the project, include important links to things like issue trackers, irc, twitter accounts if applicable.
