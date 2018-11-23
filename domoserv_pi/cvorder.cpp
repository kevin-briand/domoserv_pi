#include "cvorder.h"

CVOrder::CVOrder()
{
    wiringPiSetup();
    pinMode(CONFORT,OUTPUT);
    pinMode(ECO,OUTPUT);
    pinMode(HG,OUTPUT);
    printf("[  OK  ] wiringPi initialized\n");
}

void CVOrder::SetOutputState(int digitalIO, int state)
{
    digitalWrite(digitalIO,state);
}

void CVOrder::ChangeOrder(int order)
{
    SetOutputState(ECO,OFF);
    SetOutputState(HG,OFF);

    QString nameActualOrder,nameNewOrder;
    switch(CVState) {
    case confort:
        nameActualOrder = "Confort";
    case eco:
        nameActualOrder = "Eco";
    case horsGel:
        nameActualOrder = "Hors gel";
    }

    switch(order) {
    case confort:
        nameNewOrder = "Confort";
        break;
    case eco:
        SetOutputState(ECO,ON);
        nameNewOrder = "Eco";
        break;
    case HG:
        SetOutputState(HG,ON);
        nameNewOrder = "Hors gel";
        break;
    default:
        break;
    }

    CVState = order;

    if(nameNewOrder.isEmpty())
        printf("order change error\n");
    else
        printf("Change order " + nameActualOrder.toLatin1() + " to " + nameNewOrder.toLatin1() + "\n");
}

void CVOrder::ReceiptDataFromUser(QTcpSocket *user, QString data)
{
    if(data.split("|").count() != 2)
        printf("Data corrupted !\n");
    else
    {
        if(data.split("|").at(0) == "CVOrder")
            ChangeOrder(data.split("|").at(1).toInt());
    }
}
