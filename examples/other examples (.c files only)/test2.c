// test_edge_cases.c: Focus on tricky syntax.

int main(){int x=10;int y=20;int z=0;
    // Test compound and tricky operators with no spaces
    z=x*y;
    y+=++x; // Pre-increment and compound assignment
    @
    /* Test bitwise and logical operators */
    int a=1,b=2;
    if(a==1&&b!=0){
        a<<=1; // Bitwise left shift assignment
    }
    0.2222.3333
    333333333
    456
    // A number starting with a decimal point is not standard C,
    // but the scanner should handle "0.5" properly.
    float f = 0.5;

    return (x&y)|z; // Return with bitwise operators
}