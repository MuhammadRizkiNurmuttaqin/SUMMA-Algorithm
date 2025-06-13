1. Siapkan terlebih dahulu file file yang diperlukan seperti: a. Dockerfile b. Docker-compose.yml c. Entrypoint.sh d. Hostfile e. P12_051.c
2. Setelah itu build docker dengan cara “docker compose build”
3. Docker compose up -d
4. Setelah itu copy file hostfile ke semua node dengan cara “docker cp hostfile node{id}:/home/mpiuser/hostfile”
5. Lalu setelah itu untuk semua node lakukan seperti ini
    docker exec -it node{id} bash
    su - mpiuser
    ssh-keygen -t rsa -N "" -f ~/.ssh/id_rsa
    ssh-copy-id mpiuser@node{id} ~lakukan copy id ke semua id
6. setelah itu cek sambungan dengan setiap node dengan cara, contoh “ssh mpiuser@node{id}”
7. Copy file matriks A dan matriks B kedalam file sehingga bisa terbaca oleh execution file dengan cara "docker cp matrixA.csv node1:/home/mpiuser/"
8. masuk ke node1 dengan cara docker exec -it node1 bash
9. lalu su – mpiuser
10. jalankan programmnya “mpirun -np {jumlah proses} ./p13_051 {jumlah order matriks}”
11. contohnya "mpirun -np 4 ./p13_051 100"
maka nanti akan muncul waktu eksekusinya dan keterangan bahwa file hasil sudah tersimpan di matrixC.csv
