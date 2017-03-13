# seqno increment

among different processes

## Structure

- server

  > 1. provide initial share memory  
  > 2. flush seq into file regularly


- client

  > typically lock share mem & increase seq  
  > > base on `fcntl(file lock)`记录锁

## TODO

- check seqno incremental logic timing:

  1. using `atol() -> ++ -> sprintf`

  2. using `alpha increase with loop`

     > ```c
     > char *increbufl(char *rbuf,int rlen,int wlen)
     > {
     >     /* 读出数字串长, 写入数字串长 */
     >     int i,j,c;
     >     int npp = 1;
     >
     >     for (i=rlen-1,j=wlen-1;i>=0&&j>=0;i--,j--)
     >     {
     >         c = *(rbuf+i)+npp;
     >         if (c>'9')
     >         {
     >             npp = 1;
     >             *(rbuf+j) = '0';
     >         }
     >         else
     >         {
     >             npp = 0;
     >             *(rbuf+j) = c;
     >         }
     >     }
     >     return rbuf;
     > }
     > ```
     >
