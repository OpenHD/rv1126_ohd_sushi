 #include <stdio.h>
#include <stdlib.h>

#include <sodium.h>

#include <pcap/pcap.h>

static void iteratePcapTimestamps(pcap_t* ppcap){
        int* availableTimestamps;
        const int nTypes=pcap_list_tstamp_types(ppcap,&availableTimestamps);
        //std::cout<<"N available timestamp types "<<nTypes<<"\n";
        for(int i=0;i<nTypes;i++){
            const char* name=pcap_tstamp_type_val_to_name(availableTimestamps[i]);
            const char* description=pcap_tstamp_type_val_to_description(availableTimestamps[i]);
            //std::cout<<"Name: "<<std::string(name)<<" Description: "<<std::string(description)<<"\n";
            if(availableTimestamps[i]==PCAP_TSTAMP_HOST){
                //std::cout<<"Setting timestamp to host\n";
                pcap_set_tstamp_type(ppcap,PCAP_TSTAMP_HOST);
            }
        }
        pcap_free_tstamp_types(availableTimestamps);
}


int main(int argc, char *argv[])
{
        printf("Hello World Consti! %d \n",crypto_box_NONCEBYTES);
        printf("Hello World Consti! \n");

        char p[10];
        // call sodium method
        randombytes_buf((void*)p,sizeof(p));

        printf("Done x buffer ! \n");

        return 0;
}
