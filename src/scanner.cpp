#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype>
#include <unordered_set>

using namespace std;

// A class to hold token information.
class Token {
public:
    string token_value;
    string token_class;
    int line_number;
};
int current_line=0;
// A global vector of tokens.
vector<Token> tokens;

// A boolean variable to indicate whether or not an unexpected character error occurs.
bool unexpected_char_error = false;
bool multi_decimal_points = false;
char unexpected_char;
bool unterminated_comment_error = false;
string multi_digit_numeric_const ="";

//SCANNER FUNCTION IMPLEMENTATION

//  1-  A helper function to add a new token to the global list
void addToken(const string& value, const string& type,int linenum) {
    Token newToken;
    newToken.token_value = value;
    newToken.token_class = type;
    newToken.line_number=linenum;
    tokens.push_back(newToken);
}

// 2- Function to scan the source code string and generate tokens
void scan(const string& source_code) 
    {
    // A pointer (using an index for safety) to the current character
    int current_char_index = 0;
    //int current_line = 1; //we've it a global variaible 
    // Predefined lists for keywords, operators, and special characters
    const unordered_set<string> keywords = {
        "auto", "break", "case", "char", "const",
        "continue", "default", "do", "double", "else",
        "enum", "extern", "float", "for", "goto", "if", 
        "int", "long", "register", "return", "short", "signed",
        "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned",
        "void", "volatile","while"
    };
    const unordered_set<char> single_char_operators = {'+', '-', '*', '/', '=', '<', '>','%','^', '|' , '&','~', '!'};
    const unordered_set<string> multi_char_operators = {"++", "--","<<",">>",  "==", "&&", "||",  "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", "!=", ">=", "<=","pow"};
    const unordered_set<char> special_chars = {'(', ')', '{', '}', ';', ',', '#',  '.', '[' , ']'};
        if(source_code.empty())
                {
                current_line=0;
                return;
                }else current_line=1;
    // Loop through the entire source code string
    while (current_char_index < source_code.length())
        {
        char currentChar = source_code[current_char_index];
        
        // ---------------------------------
        // Check 1: WHITESPACE
        // ---------------------------------
        
        if (currentChar == '\n') {
            current_line++;
            current_char_index++;
            continue;
        }
        else if (isspace(currentChar)) {
            current_char_index++;
            continue;
        }
         // Ignore and move to the next character
        // ---------------------------------
        // Check 2: COMMENTS (starting with /)
        // ---------------------------------
        if (currentChar == '/') 
        {
            // Check for single-line or multi-line comment
            if (current_char_index + 1 < source_code.length())
                {
                char nextChar = source_code[current_char_index + 1];
                // Case A: Single-line comment (//)
                if (nextChar == '/') 
                    {
                    // CAPTURE the line number where the comment starts.
                    int start_line = current_line;
                    
                    // Advance the pointer past the end of the line.
                    // Skip characters until a newline is found
                    while (current_char_index < source_code.length() && source_code[current_char_index] != '\n') 
                        {
                           
                            current_char_index++;
                        } 
                        addToken("//" ,"Single-Line Comment",start_line);
                        //current_line++;--> handles in the whitespaces 
                    
                    continue; // Comment ignored, continue main loop
                    }
                // Case B: Multi-line comment (/*)
                else if (nextChar == '*') 
                {   
                     // CAPTURE the line number where the comment starts.
                    int start_line = current_line;
                    current_char_index +=2; // Move past '/*'
                    while (current_char_index + 1 < source_code.length() &&
                            !(source_code[current_char_index] == '*' && source_code[current_char_index + 1] == '/'))
                                {
                                    if (source_code[current_char_index] == '\n') 
                                    {
                                        current_line++;
                                    }
                                current_char_index++;
                                }
                                  // Check if we exited the loop because of EOF, which is an error.
                    if (current_char_index + 1 >= source_code.length()) {
                        // SET THE NEW, SPECIFIC ERROR FLAG
        unterminated_comment_error = true;
        break; // Exit the main scan loop.
                    }            
                    current_char_index += 2; // Move past '*/'
                    addToken("/* .. */" ,"Multi-Line Comment",start_line);
                    continue; // Comment ignored, continue main loop
                }
                }
            // If not a comment, it's a division operator (handled below)
        }
        // ---------------------------------
        // Check 3: PREPROCESSOR DIRECTIVES (like #include)
        // ---------------------------------
        if (currentChar == '#') 
        {
            string directive;
            while (current_char_index < source_code.length() && source_code[current_char_index] != '\n') {
                directive += source_code[current_char_index];
                current_char_index++;
            }
            addToken(directive, "PREPROCESSOR DIRECTIVE",current_line);
            continue;
        }

        // ---------------------------------
        // Check 4: OPERATORS & SPECIAL CHARACTERS
        // ---------------------------------
        // Check for MULTI-character operators
        
        // A: Check for TRIPLE-character operators
        if (current_char_index + 2 < source_code.length())
        { 
            string triple_char_op ="0";
            triple_char_op = source_code.substr(current_char_index, 3);
            
                
            if ( multi_char_operators.find(triple_char_op) != multi_char_operators.end())
                        {
                        addToken(triple_char_op, "OPERATOR",current_line);
                        current_char_index += 3;
                        continue;
                        }
        }
        // B: Check for DOUBLE-character operators
        if (current_char_index +1 < source_code.length())
        {   
            string double_char_op ="0";
            double_char_op = source_code.substr(current_char_index, 2);
            if ( multi_char_operators.find(double_char_op) != multi_char_operators.end())
                        {
                        addToken(double_char_op, "OPERATOR",current_line);
                        current_char_index += 2;
                        continue;
                        }
        }
            
                    
                    
        // Check for SINGLE-character operators (one-char-long)
            if (single_char_operators.find(currentChar)!= single_char_operators.end())
                    {
                    string currentChar_string (1, currentChar);
                    addToken(currentChar_string, "OPERATOR",current_line);
                    current_char_index ++;
                    continue;
                    }
            // Check for SPECIAL CHARACTERS (one-char-long)
                else if ((special_chars.find(currentChar)!= special_chars.end()))
                    {
                    string currentChar_string (1, currentChar);
                    
                    addToken(currentChar_string, "SPECIAL CHARACTER",current_line);
                    if (currentChar=='\'' && isalnum(source_code[current_char_index+1]) && !isalnum(source_code[current_char_index+2] ) && source_code[current_char_index+2] != '_')
                        {
                            string char_literal;
                            char_literal +=source_code[current_char_index+1];
                            addToken(char_literal,"CHAR_LITERAL",current_line);
                            current_char_index ++;
                        }
                    current_char_index ++;
                    continue;
                    }
        
        
        // ---------------------------------
        // Check 5: IDENTIFIERS and KEYWORDS
        // ---------------------------------
        if (isalpha(currentChar) || currentChar == '_')
            {
            string word;
            // Keep reading characters until the word is finished
            while (current_char_index < source_code.length() && (isalnum(source_code[current_char_index]) || source_code[current_char_index] == '_')) {
                word += source_code[current_char_index];
                current_char_index++;
            }
            
            // Compare the word with our keywords list
            if (keywords.count(word)) {
                addToken(word, "KEYWORD",current_line);
            } else {
                addToken(word, "IDENTIFIER",current_line);
            }
            continue;
        }

        // ---------------------------------
        // Check 6: NUMERIC CONSTANTS
        /*
            WE HAVE 2 SCENARIOS ON ENCOUNTERING :
            MULTIPLE DECIMAL POINTS WITHIN THE SAME NUMBER 
            
            -->FIRST ONE IS TO CONSIDER THE WHOLE NUMERIC CONSTANT WITH
            MORE THAN ONE DECIMAL POINT (i.e., 0.2222.333 ) 
            AN RECOGNIZED (UNEXPECTED / DISALLOWED) TOKEN 
            
            --> SECOND ONE (ASSUMING ANY GENERAL CASE 
            WITH ANY NO. OF DECIMAL POINTS FOUND)  IS TO 
            
            CONSIDER THE WHOLE PART 
            BEFORE THE SECOND DECIMAL POINT AS A TOKEN OF NUMERIC CONSTANT CLASS,
            STARTING FROM THE SECOND DECIMAL POINT TILL LAST DIGIT BEFORE THE THIRD ONE
            AS A TOKEN OF NUMERIC CONSTANT CLASS, 
            STARTING FROM THE THIRD DECIMAL POINT TILL LAST DIGIT BEFORE THE FOURTH ONE
            AS A TOKEN OF NUMERIC CONSTANT CLASS, AND SO ON...

        */
        // ---------------------------------

        // SCENARIO #1
/*
            if (isdigit(currentChar) || (currentChar == '.' && isdigit(source_code[current_char_index + 1])))
            {
            string number;
            bool hasDecimal = false;
            int save_start_index = current_char_index;
            while (current_char_index < source_code.length() && (isdigit(source_code[current_char_index]) || source_code[current_char_index] == '.')) 
                {
                    
                if (source_code[current_char_index] == '.')
                    {
                        if (hasDecimal) 
                        {
                            
                            multi_decimal_points= true;
                            current_char_index=save_start_index;
                            while(source_code[current_char_index] =='.'|| isdigit(source_code[current_char_index]))
                                {
                                    multi_digit_numeric_const =+source_code[current_char_index];
                                    current_char_index++;
                                }
                            break; // Break if the numeric constant constains more than one decimal point
                        }
                        hasDecimal = true;
                    }
                number += source_code[current_char_index];
                current_char_index++;
                }
            if (multi_decimal_points) break;
            addToken(number, "NUMERIC CONSTANT");
            continue;
        }
        
*/

        //-------------------------------

        //SCENARIO #2
        


        //-------------------------------------
        if (isdigit(currentChar) || (currentChar == '.' && isdigit(source_code[current_char_index + 1])))
            {
            string number;
            bool has_radix_point = false;
            while (current_char_index < source_code.length() && (isdigit(source_code[current_char_index]) || source_code[current_char_index] == '.')) 
                {
                    
                    if (source_code[current_char_index] == '.')
                    
                    {
                        has_radix_point=true;
                        number += source_code[current_char_index];
                        current_char_index++;
                        while (current_char_index < source_code.length() && (isdigit(source_code[current_char_index])))
                                {
                                    number += source_code[current_char_index];
                                    current_char_index++;
                                }
                                
                                addToken(number, "NUMERIC CONSTANT",current_line);
                                number={};
                                continue;       
                    
                    }

                    number += source_code[current_char_index];
                    current_char_index++;
                }
            
            add_to_tokens:
            if( !has_radix_point )
            {
                addToken(number, "NUMERIC CONSTANT",current_line);
                
            }
            continue;
            }
        //------------------------------------

        // ---------------------------------
        // Check 7: UNEXPECTED CHARACTERS (ERROR)
        // ---------------------------------
        /* addToken(string(1, currentChar,current_line), "ERROR: UNEXPECTED CHARACTER");
        cerr << "Error: Unexpected character '" << currentChar << "' found." << endl;
        current_char_index++; // Move past the error character
        */
        unexpected_char= source_code[current_char_index]; 
        unexpected_char_error= true;
        break;
    }
    }


int main() {
    // getting the .c file from the user 
    char choice;
    string file_path;
    again : 
    cout << "Is the .c file in the same directory as this program? (y/n): ";
    cin >> choice;
    cout<<endl;
    cin.ignore(); // Clear the input buffer before getting the path/filename
    if (tolower(choice) == 'n') {
        cout << "Please enter the full path to the .c file: ";
    } else {
        cout << "Please enter the name of the .c file: ";
    }
    getline(cin, file_path);
    cout<<endl;
    ifstream input_file(file_path);
        if (!input_file.is_open())
            {
            cerr << "Error: Could not open file '" << file_path << "'" << endl;
            cout<< "Please check and try again to enter the right name / path of the .c file to scan."<<endl;
            goto again; 
            }
    // Read the entire .c file content into a single string
        string source_code((istreambuf_iterator<char>(input_file)), istreambuf_iterator<char>());
        input_file.close();
    // Scan the code to populate the global 'tokens' vector
        scan(source_code);
        if (source_code.empty() )
       {
        cout<<endl<< "your source C-program is empty.. no code to scan"<<endl;
        return 1;
       }
      // Add this new error check
if (unterminated_comment_error) {
    cout << "ERROR: Unterminated multi-line comment at end of file!" << endl;
    cout << "click enter to end the program";
    cin.get();
    return 1;
}


    // check that  there're no errors that prevents us from having a suitable output file 
        //1 - unexpected char
        if (unexpected_char_error)
            { 
                cout<<"ERROR : AN UNEXPECTED CHARACTER '"<<unexpected_char<<"'IS FOUND!! at line #"<<current_line<<endl<<
                "click enter to end the program";
            cin.get();
                return 1;
            }
        //2- numeric constant with more than one decimal point
        /*
        if (multi_decimal_points)
            { 
                cout<<"ERROR : NUMERIC CONSTANT WITH MORE THAN ONE DECIMAL POINT IS FOUND!!"<<endl<<
                "click enter to end the program";
            cin.get();
                return 1;
            }
        */
       
    // Finally ALL GOES FINE , our scanner should output a .txt file. 
    //For now, we'll name it "tokens.txt" 
        ofstream output_file("tokens.txt");
        if (!output_file.is_open())
            {
            cerr << "Error: Could not create output file 'tokens.txt'" << endl;
            return 1;
            }
    
    // Write the tokens to the file in the specified format
        for (const auto& token : tokens)
            {
            output_file << "<" << token.token_class << ", " << token.token_value << ", " << token.line_number <<">" << endl;
            }
        output_file.close();

        cout << "Scanning complete."<<endl<<" Output written to tokens.txt" <<endl<<
        "Kindly note that the output (the .txt file) is located at the same directory as this C++ programm." 
        <<endl<<"the size of your source C-program in lines is : "<<current_line<<"  line(s)"<<endl<< "All done .. click enter to end the program"<<endl;
                
        cin.get();

        return 0;

}