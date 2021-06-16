#docker run --rm -it -p 8090:8090 -e HOST=postgis -e PORT=5432 -e POSTGRES_DBNAME=tracking -e TBL=public.llh_buoy -e POSTGRES_USER=physalia -e POSTGRES_PASS='physalia_pswd' physalia_tracking bash

#https://waytolearnx.com/2020/06/socket-python-serveur-avec-plusieurs-clients.html
import socket, ssl
import psycopg2
import select
import os
from datetime import datetime
from threading import Thread 
from socketserver import ThreadingMixIn 

t_host = os.getenv('HOST','localhost')
t_port = os.getenv('PORT','5432')
t_dbname = os.getenv('POSTGRES_DBNAME','tracking')
t_user = os.getenv('POSTGRES_USER','physalia_user')
t_pw = os.getenv('POSTGRES_PASS','physalia_pswd!')
t_tbl = os.getenv('TBL','public.llh_buoy') 

context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)

conn = psycopg2.connect(host=t_host, port=t_port, dbname=t_dbname, user=t_user, password=t_pw)
cur = conn.cursor()

class myThread(Thread):

    def __init__(self,ip,port): 
        Thread.__init__(self) 
        self.ip = ip 
        self.port = port 
        print ("[+] Nouveau thread démarré pour " + ip + ":" + str(port))
 
    def run(self):
        while True : 
            data = con.recv(2048).decode()
            if len(data) == 0: break
            #print("Le serveur a reçu des données:", data)
            vL = data.split()
            s = ""
            s += "INSERT INTO " + t_tbl +" ("
            s += " dat,lati,longi,height,q,ns,sdn,sde,sdu,sdne,sdeu,sdun,age,ratio,buoy_name"
            s += ") VALUES ("
            s += "(%s),(%s),(%s),(%s),(%s),(%s),(%s),(%s),(%s),(%s),(%s),(%s),(%s),(%s),(%s)"
            s += ")"
            try:
                #print(vL[0] + ' ' + vL[1])
                dt_tm = str(vL[0]) + ' ' + str(vL[1])
                date_time_obj = datetime.strptime(dt_tm, '%Y/%m/%d %H:%M:%S.%f')
                #print(date_time_obj)
                cur.execute(s,(date_time_obj,vL[2],vL[3],vL[4],vL[5],vL[6],vL[7],vL[8],vL[9],vL[10],vL[11],vL[12],vL[13],vL[14],vL[15]))
                conn.commit()
                #print('data inject!')
                #break
            except psycopg2.Error as e:
                t_msg_err = "SQL error: " + e + "/n SQL: " #+ s
                return render_template("error.html", t_msg_err = t_msg_err)
            break
            #cur.close()


# Programme du serveur TCP
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) 
s.bind(('0.0.0.0', 8090))
mythreads = [] 
 
while True: 
    s.listen(10) 
    print("Serveur: en attente de connexions des clients TCP ...")  
    (con, (ip,port)) = s.accept() 
    mythread = myThread(ip,port) 
    mythread.start() 
    mythreads.append(mythread) 
for t in mythreads: 
    t.join()
