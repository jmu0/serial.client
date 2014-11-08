#define name "arduino02"
#define coml 20
#define devl 20
#define portl 20
#define vall 5
#define eventCount 20

char com[coml];
int ncom = 0;
char dev[devl];
int ndev = 0;
char port[portl];
int nport = 0;
char val[vall];
int nval = 0;
char stat = 'c';
int pinStatus[14];
struct event {
    bool isset;
    char ad;
    int pin;
    char op;
    int val;
    int prev;
} events[eventCount];

void nextStat();
void doCommand();
int port2int();
int val2int();
void printStatus();
void setEvent();
void resetEvent();
void checkEvents();
void raiseEvent(int* i);
void printEvents();

void setup()
{
    Serial.begin(9600);
    for (int i = 0; i < 14; i++)
    {
        pinMode(i, OUTPUT);
        digitalWrite(i, LOW);
        pinStatus[i]=0;
    }
    pinMode(A0, INPUT);
    pinMode(A1, INPUT);
    pinMode(A2, INPUT);
    pinMode(A3, INPUT);
    pinMode(A4, INPUT); 
    pinMode(A5, INPUT);
    Serial.print("init ");
    Serial.println(name);
}

void loop()
{
    while(Serial.available())
    {
        char c = Serial.read();
        //Serial.print(c);
        if (c == '\n' || c == '\r')
        {
            com[ncom]='\0';
            port[nport]='\0';
            val[nval]='\0';
            doCommand();
            stat = 'c';
            ncom = 0;
            ndev = 0;
            nport = 0;
            nval = 0;
        }
        else if ((c == ' ') || (c == '=' && stat == 'p'))
        {
            nextStat();
        }
        else
        {
            if (stat == 'c' && ncom < coml)
            {
                com[ncom++] = c;
            }
            else if (stat == 'd' && ndev < devl)
            {
                dev[ndev++] = c;
            }
            else if (stat == 'p' && nport < portl)
            {
                port[nport++] = c;
            }
            else if (stat == 'v' && nval < vall)
            {
                val[nval++] = c;
            }
        }
    }
    checkEvents();
    delay(50);
}

void nextStat()
{
    if (stat == 'c')
    {
        stat = 'd';
    }
    else if (stat == 'd')
    {
        stat = 'p';
    }
    else if (stat == 'p')
    {
        stat = 'v';
    }
    else
    {
        stat = 'x';
    }
}

void doCommand()
{
    if (strcmp(com, "setcontrol") == 0)
    {
        int p = port2int();
        if (port[0]=='d' && port[1] == 'i')
        {
            if (nval > 0)
            {
                if (val[0] == '1')
                {
                    digitalWrite(p, HIGH);
                    pinStatus[p]=1;
                }
                else
                {
                    digitalWrite(p, LOW);
                    pinStatus[p]=0;
                }
            }
            else
            {
                Serial.println("ERROR: incorrect value");
            }
        }
        else if (port[0] == 'a' && port[1] == 'n')
        {
            if (nval > 0)
            {
                pinStatus[p] = val2int();
                analogWrite(p, pinStatus[p]);
            }
            else
            {
                Serial.println("ERROR: incorrect value");
            }
        }
    }
    else if (strcmp(com, "setevent") == 0)
    {
        setEvent();
    }
    else if (strcmp(com, "resetevent") == 0)
    {
        resetEvent();
    }
    else if (strcmp(com, "requeststatus") == 0)
    {
        printStatus();
    }
    else if (strcmp(com, "ping") == 0)
    {
        Serial.print("pong ");
        Serial.println(port);
    }
    else if (strcmp(com, "printevents") == 0)
    {
        printEvents();
    }
    else
    {
        Serial.println("Command not found");
    }
    /*
    Serial.print("INFO: com:");
    Serial.print(com);
    Serial.print(",dev:");
    Serial.print(dev);
    Serial.print(",port:");
    Serial.print(port);
    Serial.print(",val:");
    Serial.println(val);
    */
}

void printStatus()
{
    Serial.print("returnstatus ");
    Serial.print(port);
    Serial.print(" {\"name\":\"");
    Serial.print(name);
    Serial.print("\",\"status\":{");
    for (int analogPin = 0; analogPin < 6; analogPin++) {
        int pinReading = analogRead(analogPin);
        Serial.print("\"an");
        Serial.print(analogPin);
        Serial.print("\":\"");
        Serial.print(pinReading);
        Serial.print("\"");
        Serial.print(",");       
    }
    for (int digitalPin = 0; digitalPin < 14; digitalPin++)
    {
        Serial.print("\"di");
        Serial.print(digitalPin);
        Serial.print("\":\"");
        Serial.print(pinStatus[digitalPin]);
        Serial.print("\"");
        if (digitalPin < 13) Serial.print(",");
    }
    Serial.println("}}");
}

void setEvent()
{
    for (int i = 0; i < eventCount; i++)
    {
        if (events[i].isset == false)
        {
            events[i].isset = true;
            events[i].ad = port[0];
            events[i].pin = atoi(&(port[2]));
            if (val[0] == '=' || val[0] == '<' || val[0] == '>')
            {
                events[i].op = val[0];
            }
            else
            {
                events[i].op = '=';
            }
            events[i].val = atoi(&(val[1]));
            events[i].prev = -1;
            break;
        }
    }
}

void resetEvent()
{
    for (int i = 0; i < eventCount; i++)
    {
        if (events[i].isset == true)
        {
            if ((events[i].ad == port[0]) 
                    && (events[i].pin == atoi(&(port[2]))) 
                    && (events[i].op == val[0]) 
                    && (events[i].val == atoi(&(val[1]))))
            {
                events[i].isset = false;
                events[i].ad = ' ';
                events[i].pin = 0;
                events[i].op = ' ';
                events[i].val = 0;
                events[i].prev = -1;
                break;
            }
        }
    }
}

void checkEvents()
{
    for (int i = 0; i < eventCount; i++)
    {
        if (events[i].isset == true)
        {
            int readval = -1;
            if (events[i].ad == 'a')
            {
                readval = analogRead(events[i].pin);
            }
            else
            {
                readval = pinStatus[events[i].pin];
            }
            switch (events[i].op)
            {
                case '=':
                    if ((readval == events[i].val) && (events[i].prev < 1))
                    {
                        Serial.print("readval ");
                        Serial.println(readval);
                        raiseEvent(&i);
                        events[i].prev = 1;
                    }
                    else if ((readval != events[i].val) && (events[i].prev ==1))
                    {
                        Serial.print("readval ");
                        Serial.println(readval);
                        events[i].prev = 0;
                    }
                    break;
                case '<':
                    if (readval < events[i].val && events[i].prev < 1)
                    {
                        Serial.print("readval ");
                        Serial.println(readval);
                        raiseEvent(&i);
                        events[i].prev = 1;
                    }
                    else if ((readval >= events[i].val) && (events[i].prev ==1))
                    {
                        Serial.print("readval ");
                        Serial.println(readval);
                        events[i].prev = 0;
                    }
                    break;
                case '>':
                    if (readval > events[i].val && events[i].prev < 1)
                    {
                        Serial.print("readval ");
                        Serial.println(readval);
                        raiseEvent(&i);
                        events[i].prev = 1;
                    }
                    else if ((readval <= events[i].val) && (events[i].prev ==1))
                    {
                        Serial.print("readval ");
                        Serial.println(readval);
                        events[i].prev = 0;
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

void raiseEvent(int* i)
{
    Serial.print("event ");
    if (events[*i].ad == 'a')
    {
        Serial.print("an");
    }
    else
    {
        Serial.print("di");
    }
    Serial.print(events[*i].pin);
    Serial.print(" ");
    Serial.print(events[*i].op);
    Serial.println(events[*i].val);
}

void printEvents()
{
    Serial.println();
    for (int i = 0; i < eventCount; i++)
    {
        Serial.print("isset:");
        Serial.print(events[i].isset);
        Serial.print(",ad:");
        Serial.print(events[i].ad);
        Serial.print(",pin:");
        Serial.print(events[i].pin);
        Serial.print(",op:");
        Serial.print(events[i].op);
        Serial.print(",val:");
        Serial.print(events[i].val);
        Serial.print(",prev:");
        Serial.println(events[i].prev);
    }
}

int port2int()
{
    if (nport == 3 || nport == 4)
    { 
        return atoi(&(port[2]));
    }
    else
    {
        return -1;
    }
}

int val2int()
{
    if (nval > 0 && nval < 5)
    {
        return atoi(&(val[0]));
    }
    else
    {
        return -1;
    }
}
