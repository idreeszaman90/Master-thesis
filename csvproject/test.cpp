#include <stdio.h>


int dayofweek(int d, int m, int y)
{
    static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    y -= m < 3;
    return ( y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}
 
/* Driver function to test above function */
int main()
{
int check=0;
    int day = dayofweek(23, 8, 2015);
check--;    
printf("%d %d", day,check);
 
    return 0;
}
