#ifdef MASTER


int intArray[NMAX_SLAVES];
int front = 0;
int rear = -1;
int itemCount = 0;

int queue_peek() {
   return intArray[front];
}

bool queue_isEmpty() {
   return itemCount == 0;
}

bool queue_isFull() {
   return itemCount == NMAX_SLAVES;
}

bool queue_Contains(int data){
  int tempArray[NMAX_SLAVES];
  int nElem = itemCount;
  bool containsData = false;

  int i = 0;
  while (!queue_isEmpty()){
    tempArray[i] = dequeue();
    if (tempArray[i] == data) containsData = true;
    i++;
  }

  for (i=0; i<nElem; i++){
    enqueue(tempArray[i]);
  }

  return containsData;
}

int queue_size() {
   return itemCount;
}

void enqueue(int data) {

   if(!queue_isFull() && !queue_Contains(data)) {
  
      if(rear == NMAX_SLAVES-1) {
         rear = -1;            
      }       

      intArray[++rear] = data;
      itemCount++;
   }
}

int dequeue() {
   int data = intArray[front++];
  
   if(front == NMAX_SLAVES) {
      front = 0;
   }
  
   itemCount--;
   return data;  
}

void queueTest(){
  enqueue(5);
  enqueue(3);
  enqueue(2);
  enqueue(6);
  Serial_Print(bDebug, "Element at front: ");
  sprintf (Sbuffer, "%u", queue_peek());
  Serial_Println(bDebug, Sbuffer);
  
  while (!queue_isEmpty()){
    sprintf (Sbuffer, "%u", dequeue());
    Serial_Println(bDebug, Sbuffer);
    
    Serial_Print(bDebug, "-");
  }
  Serial_Println(bDebug, "\n" );
  enqueue(5);
  enqueue(5);
  enqueue(3);
  enqueue(3);
  enqueue(2);
  enqueue(2);
  enqueue(6);
  enqueue(6);
  Serial_Print(bDebug, "Element at front: ");
  sprintf (Sbuffer, "%u", queue_peek());
  Serial_Println(bDebug, Sbuffer);
  while (!queue_isEmpty()){
    sprintf (Sbuffer, "%u", dequeue());
    Serial_Println(bDebug, Sbuffer);
    Serial_Print(bDebug, "-");
  }
  Serial_Println(bDebug, "\n" );
  enqueue(1);
  enqueue(2);
  enqueue(3);
  enqueue(4);
  enqueue(5);
  enqueue(6);
  enqueue(7);
  enqueue(8);
  enqueue(9);
  enqueue(10);
  Serial_Print(bDebug, "Element at front: ");
  sprintf (Sbuffer, "%u", queue_peek());
  Serial_Println(bDebug, Sbuffer);
  while (!queue_isEmpty()){
    sprintf (Sbuffer, "%u", dequeue());
    Serial_Println(bDebug, Sbuffer);
    Serial_Print(bDebug, "-");
  }
  Serial_Println(bDebug, "\n" );
}

#endif
