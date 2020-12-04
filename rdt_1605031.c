#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: SLIGHTLY MODIFIED
 FROM VERSION 1.1 of J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
       are other packets in the channel for GBN), but can be larger
   - frames can be corrupted (either the header or the data portion)
       or lost, according to user-defined probabilities
   - frames will be delivered in the order in which they were sent
       (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 1 /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "pkt" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct pkt
{
    char data[4];
};

/* a frame is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined frame structure, which all   */
/* students must follow. */
/**
 * <b>seqnum</b> - the sequence number of the frame for which payload is being sent<br>
 * <b>acknum</b> - the sequence number of the frame for which acknowledgement is being sent<br>
 * <b>checksum</b> - the checksum (sum of seqnum, acknum and the integer values of all 4 characters in the payload<br>
 * <b>payload</b> - the data to be delivered
 */
struct frm
{
    int type;
    int seqnum;
    int acknum;
    char *crcPayload;
    char payload[4];
};

/********* FUNCTION PROTOTYPES. DEFINED IN THE LATER PART******************/
void starttimer(int AorB, float increment);

void stoptimer(int AorB);

void tolayer1(int AorB, struct frm frame);

void tolayer3(int AorB, char datasent[4]);

// General global variables and macros
#define DUMMY_SEQ_OR_ACK -1

char *crcDivisor = "1011";
int divisorLength = 4, dataLength = 4, crcSteps = 0, piggyback = 0;

// Global variables for A
int seqnumToSend_A = 0;
int acknumToSend_A = DUMMY_SEQ_OR_ACK;
struct frm frmToSend_A;
bool receivedAck_A = true;

// Global variables for B
int seqnumToSend_B = 0;
int acknumToSend_B = DUMMY_SEQ_OR_ACK;
struct frm frmToSend_B;
bool receivedAck_B = true;

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
void xor (char *x) {
    int i = 0;
    // printf("XORed = ");
    for (i = 0; i < divisorLength; i++)
    {
        x[i] = (x[i] == crcDivisor[i]) ? '0' : '1';
        // printf("%c", x[i]);
    }
    // printf("\n");
}

void generateCRC(char *toBeTransmitted)
{
    if (crcSteps == 1)
        printf("generateCRC(): ENTERING.\n");
    char *toXor = (char *)malloc(sizeof(char) * (divisorLength));
    int i = 0;

    if (crcSteps == 1)
        printf("generateCRC(): Copy the first %d bits of data to the toXOR variable.\n", divisorLength);
    for (i = 0; i < divisorLength; i++)
    {
        toXor[i] = toBeTransmitted[i];
    }

    do
    {
        // printf("i = %d\n", i);
        if (toXor[0] == '1')
        {
            if (crcSteps == 1)
                printf("generateCRC(): XOR the toXOR variable with divisor if MSB of the toXOR variable is 1.\n");
            xor(toXor);
        }
        int j = 0;
        if (crcSteps == 1)
            printf("generateCRC(): Shift the toXOR variable 1 bit to the left.\n");
        for (j = 0; j < divisorLength - 1; j++)
        {
            toXor[j] = toXor[j + 1];
        }
        if (crcSteps == 1)
            printf("generateCRC(): Copy the next bit of data to the LSB of the toXOR variable.\n");
        toXor[divisorLength - 1] = toBeTransmitted[i++];
        // printf("toXOR = %s\n", toXor);
        // printf("limit = %d\n", (dataLength + divisorLength - 1));
    } while (i <= (dataLength + divisorLength - 1));

    // printf("To be transmitted = %s\n", toBeTransmitted);
    // printf("toXOR = %s\n", toXor);

    if (crcSteps == 1)
        printf("generateCRC(): Copy the generated CRC code to the last %d-1 bits of the data.\n", divisorLength);
    for (i = dataLength; i < dataLength + divisorLength - 1; i++)
    {
        toBeTransmitted[i] = toXor[i - dataLength];
    }

    if (crcSteps == 1)
        printf("generateCRC(): EXITING.\n");
    // printf("To be transmitted = %s\n", toBeTransmitted);
}

int validateCRC(const char *toBeValidated)
{
    if (crcSteps == 1)
        printf("validateCRC(): ENTERING.\n");

    char *toXor = (char *)malloc(sizeof(char) * (divisorLength));
    int i = 0;

    if (crcSteps == 1)
        printf("validateCRC(): Copy the first %d bits of data to the toXOR variable.\n", divisorLength);
    for (i = 0; i < divisorLength; i++)
    {
        toXor[i] = toBeValidated[i];
    }

    do
    {
        // printf("i = %d\n", i);
        if (toXor[0] == '1')
        {
            if (crcSteps == 1)
                printf("validateCRC(): XOR the toXOR variable with divisor if MSB of the toXOR variable is 1.\n");
            xor(toXor);
        }
        int j = 0;
        if (crcSteps == 1)
            printf("validateCRC(): Shift the toXOR variable 1 bit to the left.\n");
        for (j = 0; j < divisorLength - 1; j++)
        {
            toXor[j] = toXor[j + 1];
        }
        if (crcSteps == 1)
            printf("validateCRC(): Copy the next bit of data to the LSB of the toXOR variable.\n");
        toXor[divisorLength - 1] = toBeValidated[i++];
    } while (i <= dataLength + divisorLength - 1);

    if (crcSteps == 1)
        printf("validateCRC(): Check if calculated CRC code is composed only of 0s.\n");
    for (i = 1; i < divisorLength - 1; i++)
    {
        if (toXor[i] != '0')
        {
            // printf("%s\n", toXor);
            return 0;
        }
    }
    // printf("%s\n", toXor);
    return 1;
}

void A_output(struct pkt packet)
{
    printf("A_output(): ENTERING.\n");

    if (!receivedAck_A)
    {
        printf("A_output(): Can not send frame for %d\n", seqnumToSend_A);
        printf("A_output(): Awaiting acknowledgement for %d\n", 1 - seqnumToSend_A);
        printf("A_output(): Dropped payload = %s\n", packet.data);
        printf("A_output(): EXITING.\n");
        return;
    }
    struct frm frame;

    frame.seqnum = seqnumToSend_A;
    frame.acknum = acknumToSend_A;

    if (piggyback == 1)
    {
        frame.type = (frame.acknum == DUMMY_SEQ_OR_ACK) ? 0 : 2;
        if (frame.type == 2)
            printf("A_output(): Sending piggybacked Ack.");
        acknumToSend_A = DUMMY_SEQ_OR_ACK;
    }
    else
    {
        frame.type = 0;
    }

    printf("A_output(): Calculating CRC code and copying packet to payload.\n");
    frame.crcPayload = (char *)malloc(sizeof(char) * (divisorLength + dataLength));

    for (int i = 0; i < dataLength; i++)
    {
        frame.crcPayload[i] = packet.data[i] % 2 == 0 ? '0' : '1';
        frame.payload[i] = packet.data[i];
    }

    for (int i = dataLength; i < dataLength + divisorLength - 1; i++)
    {
        frame.crcPayload[i] = '0';
    }

    frame.crcPayload[dataLength + divisorLength - 1] = '\0';

    generateCRC(frame.crcPayload);

    printf("A_output(): Calculated CRC code and copied packet to payload.\n");

    frmToSend_A = frame;

    printf("A_output(): Starting timer.\n");
    starttimer(0, 15);
    printf("A_output(): Started timer.\n");

    printf("A_output(): CRC Payload = %s\n", frame.crcPayload);

    printf("A_output(): Transmitting frame.\n");
    tolayer1(0, frame);
    printf("A_output(): Transmitted frame.\n");

    receivedAck_A = false;

    printf("A_output(): EXITING.\n");
}

/* need be completed only for extra credit */
void B_output(struct pkt packet)
{
    printf("B_output(): ENTERING.\n");

    if (!receivedAck_B)
    {
        printf("B_output(): Can not send frame for %d\n", seqnumToSend_B);
        printf("B_output(): Awaiting acknowledgement for %d\n", 1 - seqnumToSend_B);
        printf("B_output(): Dropped payload = %s\n", packet.data);
        printf("B_output(): EXITING.\n");
        return;
    }
    struct frm frame;

    frame.seqnum = seqnumToSend_B;
    frame.acknum = acknumToSend_B;

    if (piggyback == 1)
    {
        frame.type = (frame.acknum == DUMMY_SEQ_OR_ACK) ? 0 : 2;
        if (frame.type == 2)
            printf("B_output(): Sending piggybacked Ack.");
        acknumToSend_B = DUMMY_SEQ_OR_ACK;
    }
    else
    {
        frame.type = 0;
    }

    printf("B_output(): Calculating CRC code and copying packet to payload.\n");
    frame.crcPayload = (char *)malloc(sizeof(char) * (divisorLength + dataLength));

    for (int i = 0; i < dataLength; i++)
    {
        frame.crcPayload[i] = packet.data[i] % 2 == 0 ? '0' : '1';
        frame.payload[i] = packet.data[i];
    }

    for (int i = dataLength; i < dataLength + divisorLength - 1; i++)
    {
        frame.crcPayload[i] = '0';
    }

    frame.crcPayload[dataLength + divisorLength - 1] = '\0';

    generateCRC(frame.crcPayload);

    printf("B_output(): Calculated CRC code and copied packet to payload.\n");

    frmToSend_B = frame;

    printf("B_output(): Starting timer.\n");
    starttimer(0, 15);
    printf("B_output(): Started timer.\n");

    printf("B_output(): CRC Payload = %s\n", frame.crcPayload);

    printf("B_output(): Transmitting frame.\n");
    tolayer1(0, frame);
    printf("B_output(): Transmitted frame.\n");

    receivedAck_B = false;

    printf("B_output(): EXITING.\n");
}

/* called from layer 3, when a frame arrives for layer 4 */
void A_input(struct frm frame)
{
    printf("A_input(): ENTERING.\n");

    printf("A_input(): Checking validity of CRC code.\n");

    if (!validateCRC(frame.crcPayload))
    {
        printf("A_input(): CRC code is invalid.\n");
        printf("A_input(): CRC Payload = %s\n", frame.crcPayload);
        printf("A_inpit(): Ignoring frame.\n");
        printf("A_input(): EXITING.\n");
        return;
    }

    printf("A_input(): CRC code is valid.\n");

    // if (frame.acknum != seqnumToSend_A)
    // {
    //     printf("A_input(): Acknum is invalid.\n");
    //     printf("A_input(): Retransmitting frame.\n");
    //     stoptimer(0);
    //     starttimer(0, 15);

    //     printf("A_output(): Payload = %s\n", frmToSend_A.payload);
    //     tolayer1(0, frmToSend_A);

    //     printf("A_input(): Retransmitted frame.\n");
    //     printf("A_input(): EXITING.\n");
    //     return;
    // }

    if (frame.type == 0)
    {
        printf("A_input(): Discovered Data frame.\n");

        printf("A_input(): CRC Payload = %s\n", frame.crcPayload);

        if (piggyback == 1)
        {
            if (frame.seqnum == acknumToSend_A)
            {
                printf("A_input(): Data frame sequence and outstanding ack sequence same.\n");
                printf("A_input(): Transmitting Ack.\n");
                struct frm ackFrame;
                ackFrame.type = 1;
                ackFrame.seqnum = DUMMY_SEQ_OR_ACK;
                ackFrame.acknum = acknumToSend_A;

                acknumToSend_A = DUMMY_SEQ_OR_ACK;
                ackFrame.payload[0] = '0';
                ackFrame.payload[1] = '0';
                ackFrame.payload[2] = '0';
                ackFrame.payload[3] = '0';

                ackFrame.crcPayload = (char *)malloc(sizeof(char) * (dataLength + divisorLength));

                for (int i = 0; i < dataLength + divisorLength - 1; i++)
                {
                    ackFrame.crcPayload[i] = '0';
                }

                frame.crcPayload[dataLength + divisorLength - 1] = '\0';
                printf("A_input(): Calculating CRC code.\n");
                generateCRC(ackFrame.crcPayload);
                printf("A_input(): Calculated CRC code.\n");

                tolayer1(0, ackFrame);
                printf("A_input(): Transmitted Ack.\n");
            }
            else
            {
                tolayer3(0, frame.payload);
                acknumToSend_A = frame.seqnum;
                printf("A_input(): Flagged outstanding Ack.\n");
            }
        }
        else
        {
            printf("A_input(): Transmitting Ack.\n");
            struct frm ackFrame;
            ackFrame.type = 1;
            ackFrame.seqnum = DUMMY_SEQ_OR_ACK;
            ackFrame.acknum = acknumToSend_A;

            acknumToSend_A = DUMMY_SEQ_OR_ACK;
            ackFrame.payload[0] = '0';
            ackFrame.payload[1] = '0';
            ackFrame.payload[2] = '0';
            ackFrame.payload[3] = '0';

            ackFrame.crcPayload = (char *)malloc(sizeof(char) * (dataLength + divisorLength));

            for (int i = 0; i < dataLength + divisorLength - 1; i++)
            {
                ackFrame.crcPayload[i] = '0';
            }

            frame.crcPayload[dataLength + divisorLength - 1] = '\0';

            printf("A_input(): Calculating CRC code.\n");
            generateCRC(ackFrame.crcPayload);
            printf("A_input(): Calculated CRC code.\n");

            tolayer1(0, ackFrame);
            printf("A_input(): Transmitted Ack.\n");
        }
    }
    else if (frame.type == 1)
    {
        printf("A_input(): Discovered Ack frame.\n");
        printf("A_input(): Stopping timer.\n");
        stoptimer(0);
        printf("A_input(): Stopped timer.\n");
        seqnumToSend_A = 1 - seqnumToSend_A;
        receivedAck_A = true;
    }
    else if (frame.type == 2)
    {
        printf("A_input(): Discovered Piggyback frame.\n");

        printf("A_input(): CRC Payload = %s\n", frame.crcPayload);

        printf("A_input(): Stopping timer.\n");
        stoptimer(0);
        printf("A_input(): Stopped timer.\n");
        seqnumToSend_A = 1 - seqnumToSend_A;
        receivedAck_A = true;

        tolayer3(0, frame.payload);
        acknumToSend_A = frame.seqnum;
        printf("A_input(): Flagged outstanding Ack.\n");
    }

    printf("A_input(): EXITING.\n");
    return;
}

/* called when A's timer goes off */
void A_timerinterrupt(void)
{
    printf("A_timerinterrupt(): ENTERING.\n");

    if (!receivedAck_A)
    {
        printf("A_timerinterrupt(): Have not received acknowledgement for %d yet.\n", seqnumToSend_A);
        printf("A_timerinterrupt(): Retransmitting packet.\n");
        frmToSend_A.type = 0;
        starttimer(0, 15);
        printf("A_timerinterrupt(): CRC Payload = %s\n", frmToSend_A.crcPayload);
        tolayer1(0, frmToSend_A);
        printf("A_timerinterrupt(): Retransmitted packet.\n");
    }

    printf("A_timerinterrupt(): EXITING.\n");
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
    receivedAck_A = true;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a frame arrives for layer 4 at B*/
void B_input(struct frm frame)
{
    printf("B_input(): ENTERING.\n");

    printf("B_input(): Checking validity of CRC code.\n");

    if (!validateCRC(frame.crcPayload))
    {
        printf("B_input(): CRC code is invalid.\n");
        printf("B_input(): CRC Payload = %s\n", frame.crcPayload);
        printf("B_input(): Ignoring frame.\n");
        printf("B_input(): EXITING.\n");
        return;
    }

    printf("B_input(): CRC code is valid.\n");

    // if (frame.acknum != seqnumToSend_A)
    // {
    //     printf("B_input(): Acknum is invalid.\n");
    //     printf("B_input(): Retransmitting frame.\n");
    //     stoptimer(0);
    //     starttimer(0, 15);

    //     printf("A_output(): Payload = %s\n", frmToSend_A.payload);
    //     tolayer1(0, frmToSend_A);

    //     printf("B_input(): Retransmitted frame.\n");
    //     printf("B_input(): EXITING.\n");
    //     return;
    // }

    if (frame.type == 0)
    {
        printf("B_input(): Discovered Data frame.\n");

        printf("B_input(): CRC Payload = %s\n", frame.crcPayload);

        if (piggyback == 1)
        {
            if (frame.seqnum == acknumToSend_B)
            {
                printf("B_input(): Data frame sequence and outstanding ack sequence same.\n");
                printf("B_input(): Transmitting Ack.\n");
                struct frm ackFrame;
                ackFrame.type = 1;
                ackFrame.seqnum = DUMMY_SEQ_OR_ACK;
                ackFrame.acknum = acknumToSend_B;

                acknumToSend_B = DUMMY_SEQ_OR_ACK;

                ackFrame.payload[0] = '0';
                ackFrame.payload[1] = '0';
                ackFrame.payload[2] = '0';
                ackFrame.payload[3] = '0';

                ackFrame.crcPayload = (char *)malloc(sizeof(char) * (dataLength + divisorLength));

                for (int i = 0; i < dataLength + divisorLength - 1; i++)
                {
                    ackFrame.crcPayload[i] = '0';
                }
                frame.crcPayload[dataLength + divisorLength - 1] = '\0';

                printf("B_input(): Calculating CRC code.\n");
                generateCRC(ackFrame.crcPayload);
                printf("B_input(): Calculated CRC code.\n");

                tolayer1(1, ackFrame);
                printf("B_input(): Transmitted Ack.\n");
            }
            else
            {
                tolayer3(1, frame.payload);
                acknumToSend_B = frame.seqnum;
                printf("B_input(): Flagged outstanding Ack.\n");
            }
        }
        else
        {
            printf("B_input(): Transmitting Ack.\n");
            struct frm ackFrame;
            ackFrame.type = 1;
            ackFrame.seqnum = DUMMY_SEQ_OR_ACK;
            ackFrame.acknum = acknumToSend_B;

            acknumToSend_B = DUMMY_SEQ_OR_ACK;

            ackFrame.payload[0] = '0';
            ackFrame.payload[1] = '0';
            ackFrame.payload[2] = '0';
            ackFrame.payload[3] = '0';

            ackFrame.crcPayload = (char *)malloc(sizeof(char) * (dataLength + divisorLength));

            for (int i = 0; i < dataLength + divisorLength - 1; i++)
            {
                ackFrame.crcPayload[i] = '0';
            }

            frame.crcPayload[dataLength + divisorLength - 1] = '\0';

            printf("B_input(): Calculating CRC code.\n");
            generateCRC(ackFrame.crcPayload);
            printf("B_input(): Calculated CRC code.\n");

            tolayer1(1, ackFrame);
            printf("B_input(): Transmitted Ack.\n");
        }
    }
    else if (frame.type == 1)
    {
        printf("B_input(): Discovered Ack frame.\n");
        printf("B_input(): Stopping timer.\n");
        stoptimer(1);
        printf("B_input(): Stopped timer.\n");
        seqnumToSend_B = 1 - seqnumToSend_B;
        receivedAck_B = true;
    }
    else if (frame.type == 2)
    {
        printf("B_input(): Discovered Piggyback frame.\n");

        printf("A_input(): CRC Payload = %s\n", frame.crcPayload);

        printf("B_input(): Stopping timer.\n");
        stoptimer(1);
        printf("B_input(): Stopped timer.\n");
        seqnumToSend_B = 1 - seqnumToSend_B;
        receivedAck_B = true;

        tolayer3(1, frame.payload);
        acknumToSend_B = frame.seqnum;
        printf("B_input(): Flagged outstanding Ack.\n");
    }

    printf("B_input(): EXITING.\n");
    return;
}

/* called when B's timer goes off */
void B_timerinterrupt(void)
{
    printf("B_timerinterrupt(): ENTERING.\n");

    if (!receivedAck_B)
    {
        printf("B_timerinterrupt(): Have not received acknowledgement for %d yet.\n", seqnumToSend_B);
        printf("B_timerinterrupt(): Retransmitting packet.\n");
        frmToSend_B.type = 0;
        starttimer(0, 15);
        printf("B_timerinterrupt(): CRC Payload = %s\n", frmToSend_B.crcPayload);
        tolayer1(0, frmToSend_B);
        printf("B_timerinterrupt(): Retransmitted packet.\n");
    }

    printf("B_timerinterrupt(): EXITING.\n");
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void)
{
    receivedAck_A = true;
}

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
    - emulates the transmission and delivery (possibly with bit-level corruption
        and frame loss) of frames across the layer 3/4 interface
    - handles the starting/stopping of a timer, and generates timer
        interrupts (resulting in calling students timer handler).
    - generates packet to be sent (passed from later 5 to 4)

THERE IS NO REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOULD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should not have
to, and you definitely should not have to modify
******************************************************************/

struct event
{
    float evtime;       /* event time */
    int evtype;         /* event type code */
    int eventity;       /* entity where event occurs */
    struct frm *frmptr; /* ptr to frame (if any) assoc w/ this event */
    struct event *prev;
    struct event *next;
};
struct event *evlist = NULL; /* the event list */

/* possible events: */
#define TIMER_INTERRUPT 0
#define FROM_layer3 1
#define FROM_layer1 2

#define OFF 0
#define ON 1
#define A 0
#define B 1

int TRACE = 1;   /* for my debugging */
int nsim = 0;    /* number of packets from 5 to 4 so far */
int nsimmax = 0; /* number of pkts to generate, then stop */
float time = 0.000;
float lossprob;    /* probability that a frame is dropped  */
float corruptprob; /* probability that one bit is frame is flipped */
float lambda;      /* arrival rate of packets from layer 5 */
int ntolayer1;     /* number sent into layer 3 */
int nlost;         /* number lost in media */
int ncorrupt;      /* number corrupted by media*/

void init();

void generate_next_arrival(void);

void insertevent(struct event *p);

int main()
{
    struct event *eventptr;
    struct pkt pkt2give;
    struct frm frm2give;

    int i, j;
    char c;

    init();
    A_init();
    B_init();

    while (1)
    {
        eventptr = evlist; /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;
        evlist = evlist->next; /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;
        if (TRACE >= 2)
        {
            printf("\nEVENT time: %f,", eventptr->evtime);
            printf("  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                printf(", timerinterrupt  ");
            else if (eventptr->evtype == 1)
                printf(", fromlayer3 ");
            else
                printf(", fromlayer1 ");
            printf(" entity: %d\n", eventptr->eventity);
        }
        time = eventptr->evtime; /* update time to next event time */
        if (eventptr->evtype == FROM_layer3)
        {
            if (nsim < nsimmax)
            {
                if (nsim + 1 < nsimmax)
                    generate_next_arrival(); /* set up future arrival */
                /* fill in pkt to give with string of same letter */
                j = nsim % 26;
                for (i = 0; i < 4; i++)
                    pkt2give.data[i] = 97 + j;
                pkt2give.data[3] = 0;
                if (TRACE > 2)
                {
                    printf("          MAINLOOP: data given to student: ");
                    for (i = 0; i < 4; i++)
                        printf("%c", pkt2give.data[i]);
                    printf("\n");
                }
                nsim++;
                if (eventptr->eventity == A)
                    A_output(pkt2give);
                else
                    B_output(pkt2give);
            }
        }
        else if (eventptr->evtype == FROM_layer1)
        {
            frm2give.type = eventptr->frmptr->type;
            frm2give.seqnum = eventptr->frmptr->seqnum;
            frm2give.acknum = eventptr->frmptr->acknum;

            frm2give.crcPayload = (char *)malloc(sizeof(char) * (dataLength + divisorLength));

            for (i = 0; i < dataLength + divisorLength; i++)
            {
                frm2give.crcPayload[i] = eventptr->frmptr->crcPayload[i];
            }

            for (i = 0; i < 4; i++)
                frm2give.payload[i] = eventptr->frmptr->payload[i];
            if (eventptr->eventity == A) /* deliver frame by calling */
                A_input(frm2give);       /* appropriate entity */
            else
                B_input(frm2give);
            free(eventptr->frmptr); /* free the memory for frame */
        }
        else if (eventptr->evtype == TIMER_INTERRUPT)
        {
            if (eventptr->eventity == A)
                A_timerinterrupt();
            else
                B_timerinterrupt();
        }
        else
        {
            printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

terminate:
    printf(
        " Simulator terminated at time %f\n after sending %d pkts from layer3\n",
        time, nsim);
}

void init() /* initialize the simulator */
{
    int i;
    float sum, avg;
    float jimsrand();

    printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
    printf("Enter the number of packets to simulate: ");
    scanf("%d", &nsimmax);
    printf("Do you want to show crc steps? (Yes = 1, No = 0): ");
    scanf("%d", &crcSteps);
    printf("Do you want to enable piggybacking? (Yes = 1, No = 0): ");
    scanf("%d", &piggyback);
    printf("Enter generator polynomial (just type the divisor in binary): ");
    char divisor[100];
    scanf("%s", divisor);
    i = 0;
    while(divisor[i] != '\0') {
        i++;
    }
    divisorLength = i;
    crcDivisor = (char *)malloc(sizeof(char)*divisorLength);
    for (int j=0; j<divisorLength; j++) {
        crcDivisor[j] = divisor[j];
    }
    printf("Enter frame loss probability [enter 0.0 for no loss]:");
    scanf("%f", &lossprob);
    printf("Enter frame corruption probability [0.0 for no corruption]:");
    scanf("%f", &corruptprob);
    printf("Enter average time between packets from sender's layer3 [ > 0.0]:");
    scanf("%f", &lambda);
    printf("Enter TRACE:");
    scanf("%d", &TRACE);

    srand(9999); /* init random number generator */
    sum = 0.0;   /* test random number generator for students */
    for (i = 0; i < 1000; i++)
        sum = sum + jimsrand(); /* jimsrand() should be uniform in [0,1] */
    avg = sum / 1000.0;
    if (avg < 0.25 || avg > 0.75)
    {
        printf("It is likely that random number generation on your machine\n");
        printf("is different from what this emulator expects.  Please take\n");
        printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(1);
    }

    ntolayer1 = 0;
    nlost = 0;
    ncorrupt = 0;

    time = 0.0;              /* initialize time to 0.0 */
    generate_next_arrival(); /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand(void)
{
    double mmm = RAND_MAX;
    float x;          /* individual students may need to change mmm */
    x = rand() / mmm; /* x should be uniform in [0,1] */
    return (x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival(void)
{
    double x, log(), ceil();
    struct event *evptr;
    float ttime;
    int tempint;

    if (TRACE > 2)
        printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

    x = lambda * jimsrand() * 2; /* x is uniform on [0,2*lambda] */
    /* having mean of lambda        */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + x;
    evptr->evtype = FROM_layer3;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}

void insertevent(struct event *p)
{
    struct event *q, *qold;

    if (TRACE > 2)
    {
        printf("            INSERTEVENT: time is %lf\n", time);
        printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
    }
    q = evlist;    /* q points to header of list in which p struct inserted */
    if (q == NULL) /* list is empty */
    {
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else
    {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL) /* end of list */
        {
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == evlist) /* front of list */
        {
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        }
        else /* middle of list */
        {
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

void printevlist(void)
{
    struct event *q;
    int i;
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next)
    {
        printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype,
               q->eventity);
    }
    printf("--------------\n");
}

/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB /* A or B is trying to stop timer */)
{
    struct event *q, *qold;

    if (TRACE > 2)
        printf("          STOP TIMER: stopping timer at %f\n", time);
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;        /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist) /* front of list - there must be event after */
            {
                q->next->prev = NULL;
                evlist = q->next;
            }
            else /* middle of list */
            {
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    printf("Warning: unable to cancel your timer. It wasn't running.\n");
}

void starttimer(int AorB /* A or B is trying to start timer */, float increment)
{
    struct event *q;
    struct event *evptr;

    if (TRACE > 2)
        printf("          START TIMER: starting timer at %f\n", time);
    /* be nice: check to see if timer is already started, if so, then  warn */
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            printf("Warning: attempt to start a timer that is already started\n");
            return;
        }

    /* create future event for when timer goes off */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + increment;
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    insertevent(evptr);
}

/************************** TOlayer1 ***************/
void tolayer1(int AorB, struct frm frame)
{
    struct frm *myfrmptr;
    struct event *evptr, *q;
    float lastime, x;
    int i;

    ntolayer1++;

    /* simulate losses: */
    if (jimsrand() < lossprob)
    {
        nlost++;
        if (TRACE > 0)
            printf("          TOlayer1: frame being lost\n");
        return;
    }

    /* make a copy of the frame student just gave me since he/she may decide */
    /* to do something with the frame after we return back to him/her */
    myfrmptr = (struct frm *)malloc(sizeof(struct frm));
    myfrmptr->type = frame.type;
    myfrmptr->seqnum = frame.seqnum;
    myfrmptr->acknum = frame.acknum;
    myfrmptr->crcPayload = (char *)malloc(sizeof(char) * (dataLength + divisorLength));
    for (i = 0; i < dataLength + divisorLength; i++)
    {
        myfrmptr->crcPayload[i] = frame.crcPayload[i];
    }
    for (i = 0; i < 4; i++)
        myfrmptr->payload[i] = frame.payload[i];
    if (TRACE > 2)
    {
        printf("          TOlayer1: seq: %d, ack %d, check: %s ", myfrmptr->seqnum,
               myfrmptr->acknum, myfrmptr->crcPayload);
        for (i = 0; i < 4; i++)
            printf("%c", myfrmptr->payload[i]);
        printf("\n");
    }

    /* create future event for arrival of frame at the other side */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtype = FROM_layer1;      /* frame will pop out from layer1 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
    evptr->frmptr = myfrmptr;         /* save ptr to my copy of frame */
    /* finally, compute the arrival time of frame at the other end.
       medium can not reorder, so make sure frame arrives between 1 and 10
       time units after the latest arrival time of frames
       currently in the medium on their way to the destination */
    lastime = time;
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_layer1 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();

    /* simulate corruption: */
    if (jimsrand() < corruptprob)
    {
        ncorrupt++;
        myfrmptr->crcPayload[0] = myfrmptr->crcPayload[0] == '0' ? '1' : '0'; /* corrupt payload */
        if (TRACE > 0)
            printf("          TOlayer1: frame being corrupted\n");
    }

    if (TRACE > 2)
        printf("          TOlayer1: scheduling arrival on other side\n");
    insertevent(evptr);
}

void tolayer3(int AorB, char datasent[4])
{
    int i;
    if (TRACE > 2)
    {
        printf("          TOlayer3: data received: ");
        for (i = 0; i < 4; i++)
            printf("%c", datasent[i]);
        printf("\n");
    }
}
