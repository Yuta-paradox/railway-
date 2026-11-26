/*
 * B.Tech DSA Project: Railway Reservation Management System
 * Language: C (C99)
 * Core DSA: Arrays, Linear Search, Binary Search, Bubble Sort,
 *           insertion sort, queue-based waiting list.
 *
 * Build:  gcc -std=c99 -Wall -Wextra -pedantic railway_reservation.c -o railway
 * Run:    ./railway
 *
 * Demo data is loaded at startup. Bookings are kept in memory for the
 * current run; this is an educational DSA implementation, not a live railway system.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TRAINS 100
#define MAX_BOOKINGS 500
#define MAX_WAITLIST 100
#define NAME_LEN 60
#define CITY_LEN 40

typedef struct {
    int number;
    char name[NAME_LEN];
    char source[CITY_LEN];
    char destination[CITY_LEN];
    char departure[8];
    float fare;
    int totalSeats;
    int availableSeats;
} Train;

typedef struct {
    int pnr;
    int trainNumber;
    char passenger[NAME_LEN];
    int age;
    int seatNumber;       /* 0 means waiting list */
    int active;
} Booking;

typedef struct {
    int front, rear, count;
    int bookingIndex[MAX_WAITLIST];
} WaitQueue;

static Train trains[MAX_TRAINS];
static Booking bookings[MAX_BOOKINGS];
static int trainCount = 0, bookingCount = 0;
static int nextPNR = 100001;
static WaitQueue waitQueue = {-1, -1, 0, {0}};

static void readLine(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if (fgets(buffer, (int)size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
    } else {
        buffer[0] = '\0';
    }
}

static int readInt(const char *prompt) {
    char line[64], *end;
    long value;
    for (;;) {
        readLine(prompt, line, sizeof(line));
        value = strtol(line, &end, 10);
        if (end != line && *end == '\0') return (int)value;
        printf("Please enter a valid whole number.\n");
    }
}

static float readFloat(const char *prompt) {
    char line[64], *end;
    float value;
    for (;;) {
        readLine(prompt, line, sizeof(line));
        value = strtof(line, &end);
        if (end != line && *end == '\0' && value >= 0.0f) return value;
        printf("Please enter a valid non-negative amount.\n");
    }
}

static int findTrainIndexLinear(int number) {
    int i;
    for (i = 0; i < trainCount; i++)
        if (trains[i].number == number) return i;
    return -1;
}

static int findBookingIndex(int pnr) {
    int i;
    for (i = 0; i < bookingCount; i++)
        if (bookings[i].pnr == pnr && bookings[i].active) return i;
    return -1;
}

static void enqueue(int bookingIndex) {
    if (waitQueue.count >= MAX_WAITLIST) {
        printf("Waiting list is full. Booking could not be added.\n");
        return;
    }
    if (waitQueue.count == 0) waitQueue.front = 0;
    waitQueue.rear = (waitQueue.rear + 1) % MAX_WAITLIST;
    waitQueue.bookingIndex[waitQueue.rear] = bookingIndex;
    waitQueue.count++;
}

static int dequeue(void) {
    int value;
    if (waitQueue.count == 0) return -1;
    value = waitQueue.bookingIndex[waitQueue.front];
    waitQueue.front = (waitQueue.front + 1) % MAX_WAITLIST;
    waitQueue.count--;
    if (waitQueue.count == 0) waitQueue.front = waitQueue.rear = -1;
    return value;
}

static void addDemoTrains(void) {
    Train demo[] = {
        {12951, "Mumbai Rajdhani", "New Delhi", "Mumbai", "16:55", 1850.0f, 5, 5},
        {12015, "Ajmer Shatabdi", "New Delhi", "Ajmer", "06:10", 950.0f, 4, 4},
        {12988, "Ajmer Express", "Ajmer", "Sealdah", "12:30", 780.0f, 3, 3},
        {12414, "Pooja Express", "Jammu", "Ajmer", "18:20", 1120.0f, 4, 4}
    };
    size_t n = sizeof(demo) / sizeof(demo[0]);
    size_t i;
    for (i = 0; i < n && i < MAX_TRAINS; i++) trains[trainCount++] = demo[i];
}

static void displayTrain(const Train *t) {
    printf("%-7d %-22s %-15s %-15s %-8s Rs.%-8.2f %d/%d\n",
           t->number, t->name, t->source, t->destination, t->departure,
           t->fare, t->availableSeats, t->totalSeats);
}

static void listTrains(void) {
    int i;
    if (trainCount == 0) { printf("No train records available.\n"); return; }
    printf("\n%-7s %-22s %-15s %-15s %-8s %-16s %s\n",
           "Number", "Train", "Source", "Destination", "Time", "Fare", "Seats");
    for (i = 0; i < trainCount; i++) displayTrain(&trains[i]);
}

static void addTrain(void) {
    Train t;
    if (trainCount >= MAX_TRAINS) { printf("Train storage is full.\n"); return; }
    t.number = readInt("Train number: ");
    if (findTrainIndexLinear(t.number) != -1) {
        printf("That train number already exists.\n"); return;
    }
    readLine("Train name: ", t.name, sizeof(t.name));
    readLine("Source: ", t.source, sizeof(t.source));
    readLine("Destination: ", t.destination, sizeof(t.destination));
    readLine("Departure time (HH:MM): ", t.departure, sizeof(t.departure));
    t.fare = readFloat("Fare (Rs.): ");
    t.totalSeats = readInt("Total seats: ");
    if (t.totalSeats <= 0) { printf("Total seats must be positive.\n"); return; }
    t.availableSeats = t.totalSeats;
    trains[trainCount++] = t;
    printf("Train added successfully.\n");
}

static void updateTrainAvailability(int trainIndex, int delta) {
    trains[trainIndex].availableSeats += delta;
    if (trains[trainIndex].availableSeats < 0)
        trains[trainIndex].availableSeats = 0;
    if (trains[trainIndex].availableSeats > trains[trainIndex].totalSeats)
        trains[trainIndex].availableSeats = trains[trainIndex].totalSeats;
}

static void bookTicket(void) {
    int number, ti, age, index, seat;
    char passenger[NAME_LEN];
    if (bookingCount >= MAX_BOOKINGS) { printf("Booking storage is full.\n"); return; }
    number = readInt("Enter train number: ");
    ti = findTrainIndexLinear(number);
    if (ti == -1) { printf("Train not found.\n"); return; }
    readLine("Passenger name: ", passenger, sizeof(passenger));
    age = readInt("Passenger age: ");
    if (age <= 0 || age > 120) { printf("Please enter a valid age.\n"); return; }

    index = bookingCount++;
    bookings[index].pnr = nextPNR++;
    bookings[index].trainNumber = number;
    strcpy(bookings[index].passenger, passenger);
    bookings[index].age = age;
    bookings[index].active = 1;

    if (trains[ti].availableSeats > 0) {
        seat = trains[ti].totalSeats - trains[ti].availableSeats + 1;
        bookings[index].seatNumber = seat;
        updateTrainAvailability(ti, -1);
        printf("\nTicket confirmed!\nPNR: %d\nTrain: %s\nSeat: %d\nFare: Rs. %.2f\n",
               bookings[index].pnr, trains[ti].name, seat, trains[ti].fare);
    } else {
        bookings[index].seatNumber = 0;
        enqueue(index);
        printf("\nNo seats available. Added to waiting list.\nWaiting-list position: %d\nPNR: %d\n",
               waitQueue.count, bookings[index].pnr);
    }
}

static void searchTrain(void) {
    int choice, number, i, found = 0;
    char route[CITY_LEN];
    printf("1. Search by train number\n2. Search by route (source)\n");
    choice = readInt("Choose: ");
    if (choice == 1) {
        number = readInt("Train number: ");
        i = findTrainIndexLinear(number);
        if (i == -1) printf("Train not found.\n");
        else displayTrain(&trains[i]);
    } else if (choice == 2) {
        readLine("Enter source city: ", route, sizeof(route));
        for (i = 0; i < trainCount; i++) {
            if (strcmp(trains[i].source, route) == 0) {
                displayTrain(&trains[i]); found = 1;
            }
        }
        if (!found) printf("No train found from that source.\n");
    } else printf("Invalid choice.\n");
}

static void promoteWaitingPassenger(int trainNumber) {
    int i, bi;
    /* FIFO queue: promote the earliest active waiting passenger for this train.
       Non-matching entries are temporarily retained in their original order. */
    int temp[MAX_WAITLIST], tempCount = 0, promoted = 0;
    while ((bi = dequeue()) != -1) {
        if (!promoted && bookings[bi].active &&
            bookings[bi].trainNumber == trainNumber) {
            i = findTrainIndexLinear(trainNumber);
            if (i != -1 && trains[i].availableSeats > 0) {
                bookings[bi].seatNumber =
                    trains[i].totalSeats - trains[i].availableSeats + 1;
                updateTrainAvailability(i, -1);
                printf("Waiting-list passenger %s promoted. PNR: %d, Seat: %d\n",
                       bookings[bi].passenger, bookings[bi].pnr,
                       bookings[bi].seatNumber);
                promoted = 1;
            } else temp[tempCount++] = bi;
        } else temp[tempCount++] = bi;
    }
    for (i = 0; i < tempCount; i++) enqueue(temp[i]);
}

static void cancelTicket(void) {
    int pnr, bi, ti;
    pnr = readInt("Enter PNR to cancel: ");
    bi = findBookingIndex(pnr);
    if (bi == -1) { printf("Active booking not found.\n"); return; }
    bookings[bi].active = 0;
    ti = findTrainIndexLinear(bookings[bi].trainNumber);
    if (bookings[bi].seatNumber > 0 && ti != -1) {
        updateTrainAvailability(ti, 1);
        printf("Confirmed ticket cancelled.\n");
        promoteWaitingPassenger(bookings[bi].trainNumber);
    } else {
        printf("Waiting-list booking cancelled. (Inactive queue entry will be skipped.)\n");
    }
}

static void searchPassengerByPNR(void) {
    int pnr, bi;
    pnr = readInt("Enter PNR: ");
    bi = findBookingIndex(pnr);
    if (bi == -1) { printf("Active booking not found.\n"); return; }
    printf("PNR: %d | Passenger: %s | Age: %d | Train: %d | ",
           bookings[bi].pnr, bookings[bi].passenger, bookings[bi].age,
           bookings[bi].trainNumber);
    if (bookings[bi].seatNumber == 0) printf("Status: WAITING\n");
    else printf("Seat: %d | Status: CONFIRMED\n", bookings[bi].seatNumber);
}

static void bubbleSortTrainsByNumber(void) {
    int i, j;
    for (i = 0; i < trainCount - 1; i++)
        for (j = 0; j < trainCount - i - 1; j++)
            if (trains[j].number > trains[j + 1].number) {
                Train temp = trains[j];
                trains[j] = trains[j + 1];
                trains[j + 1] = temp;
            }
    printf("Trains sorted by train number (ascending).\n");
    listTrains();
}

static void insertionSortTrainsByFare(void) {
    int i, j;
    for (i = 1; i < trainCount; i++) {
        Train key = trains[i];
        j = i - 1;
        while (j >= 0 && trains[j].fare > key.fare) {
            trains[j + 1] = trains[j];
            j--;
        }
        trains[j + 1] = key;
    }
    printf("Trains sorted by fare (ascending).\n");
    listTrains();
}

static void bookingReport(void) {
    int i, confirmed = 0, waiting = 0, cancelled = 0;
    printf("\nBOOKING REPORT\n");
    printf("%-9s %-22s %-10s %-12s %s\n", "PNR", "Passenger", "Train", "Seat", "Status");
    for (i = 0; i < bookingCount; i++) {
        const char *status;
        if (!bookings[i].active) { status = "CANCELLED"; cancelled++; }
        else if (bookings[i].seatNumber == 0) { status = "WAITING"; waiting++; }
        else { status = "CONFIRMED"; confirmed++; }
        printf("%-9d %-22s %-10d %-12s %s\n", bookings[i].pnr,
               bookings[i].passenger, bookings[i].trainNumber,
               bookings[i].seatNumber == 0 ? "-" : "Allocated", status);
    }
    printf("\nConfirmed: %d | Waiting: %d | Cancelled: %d | Total records: %d\n",
           confirmed, waiting, cancelled, bookingCount);
}

static void trainDetailsMenu(void) {
    int choice;
    do {
        printf("\n--- TRAIN DETAILS MODULE ---\n1. Display trains\n2. Add train\n0. Back\n");
        choice = readInt("Choose: ");
        if (choice == 1) listTrains();
        else if (choice == 2) addTrain();
        else if (choice != 0) printf("Invalid choice.\n");
    } while (choice != 0);
}

static void bookingMenu(void) {
    int choice;
    do {
        printf("\n--- TICKET BOOKING MODULE ---\n1. Book ticket\n2. Search passenger by PNR\n0. Back\n");
        choice = readInt("Choose: ");
        if (choice == 1) bookTicket();
        else if (choice == 2) searchPassengerByPNR();
        else if (choice != 0) printf("Invalid choice.\n");
    } while (choice != 0);
}

static void cancellationSearchMenu(void) {
    int choice;
    do {
        printf("\n--- CANCELLATION AND SEARCH MODULE ---\n1. Cancel ticket\n2. Search train\n3. Search passenger by PNR\n0. Back\n");
        choice = readInt("Choose: ");
        if (choice == 1) cancelTicket();
        else if (choice == 2) searchTrain();
        else if (choice == 3) searchPassengerByPNR();
        else if (choice != 0) printf("Invalid choice.\n");
    } while (choice != 0);
}

static void sortingReportsMenu(void) {
    int choice;
    do {
        printf("\n--- SORTING AND REPORTS MODULE ---\n1. Sort by train number\n2. Sort by fare\n3. Booking report\n0. Back\n");
        choice = readInt("Choose: ");
        if (choice == 1) bubbleSortTrainsByNumber();
        else if (choice == 2) insertionSortTrainsByFare();
        else if (choice == 3) bookingReport();
        else if (choice != 0) printf("Invalid choice.\n");
    } while (choice != 0);
}

int main(void) {
    int choice;
    addDemoTrains();
    printf("============================================\n");
    printf(" RAILWAY RESERVATION MANAGEMENT SYSTEM\n");
    printf(" B.Tech DSA Mini Project | Language: C\n");
    printf("============================================\n");
    do {
        printf("\nMAIN MENU\n");
        printf("1. Train Details Module\n");
        printf("2. Ticket Booking Module\n");
        printf("3. Cancellation and Search Module\n");
        printf("4. Sorting and Reports Module\n");
        printf("5. Display All Trains\n");
        printf("0. Exit\n");
        choice = readInt("Enter your choice: ");
        switch (choice) {
            case 1: trainDetailsMenu(); break;
            case 2: bookingMenu(); break;
            case 3: cancellationSearchMenu(); break;
            case 4: sortingReportsMenu(); break;
            case 5: listTrains(); break;
            case 0: printf("Thank you for using the system.\n"); break;
            default: printf("Invalid choice. Try again.\n");
        }
    } while (choice != 0);
    return 0;
}
