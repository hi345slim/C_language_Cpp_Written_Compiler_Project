#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept> // Required for std::runtime_error

using namespace std;

// --- DATA STRUCTURES ---

// Represents a single token read from the scanner's output file.
class Token {
public:
    string token_value;
    string token_class;
    int line_number;
};

// Represents a node in our final Abstract Syntax Tree (AST).
struct ParseNode {
    string type;
    string value;
    int line;
    vector<ParseNode*> children;

    // Destructor to prevent memory leaks by cleaning up child nodes.
    ~ParseNode() {
        for (ParseNode* child : children) {
            delete child;
        }
    }
};

// --- THE PARSER CLASS ---

class Parser {
public:
    // The constructor takes the vector of tokens we will load from the file.
    Parser(const vector<Token>& tokens) : m_tokens(tokens) {}

    // The main public function to start parsing.
    ParseNode* parse() {
        try {
            return parse_program();
        } catch (const runtime_error& e) {
            // Errors are reported inside the match() function.
            // We return nullptr to signal that parsing failed.
            return nullptr; 
        }
    }

private:
    const vector<Token>& m_tokens;
    size_t m_current_pos = 0;

    // --- UTILITY METHODS ---

    const Token& peek() {
        // Automatically skip over any comment tokens from the scanner.
        while (!is_at_end() && (m_tokens[m_current_pos].token_class == "Single-Line Comment" || 
                m_tokens[m_current_pos].token_class == "Multi-Line Comment")) {
                m_current_pos++;
        }
        return m_tokens[m_current_pos];
    }

    void advance() {
        if (!is_at_end()) m_current_pos++;
    }

    bool is_at_end() {
        // Check if we are past the last valid token index.
        return m_current_pos >= m_tokens.size();
    }

    // The core of the parser: consumes a token if it matches what we expect.
    Token match(const string& expected_class, const string& expected_value = "") {
        const Token& token = peek();
        if (token.token_class == expected_class && (expected_value.empty() || token.token_value == expected_value)) {
            Token matched_token = token;
            advance();
            return matched_token;
        }
        
        string error_message = "Expected " + expected_class;
        if (!expected_value.empty()) error_message += " with value '" + expected_value + "'";
        error_message += ", but got " + token.token_class + " with value '" + token.token_value + "'";
        
        report_error(error_message);
        throw runtime_error("Syntax Error");
    }

    // --- ERROR REPORTING ---
    void report_error(const string& message) {
        if (is_at_end()) {
             cerr << "[End of File] Syntax Error: " << message << endl;
        } else {
             cerr << "[Line " << peek().line_number << "] Syntax Error: " << message << endl;
        }
    }

    // --- RECURSIVE DESCENT PARSING FUNCTIONS ---

    // Rule: program -> top_level_declaration*
    ParseNode* parse_program() {
        ParseNode* program_node = new ParseNode{"Program", "", (m_tokens.empty() ? 0 : peek().line_number)};

        while (!is_at_end()) {
            program_node->children.push_back(parse_top_level_declaration());
        }

        cout << "Parsing completed successfully." << endl;
        return program_node;
    }

    // Rule: top_level_declaration -> preprocessor | variable_declaration | function_definition ...
    ParseNode* parse_top_level_declaration() {
        if (peek().token_class == "PREPROCESSOR DIRECTIVE") {
            Token directive = match("PREPROCESSOR DIRECTIVE");
            return new ParseNode{"PreprocessorDirective", directive.token_value, directive.line_number};
        }

        if (peek().token_value == "const" || peek().token_value == "int" || 
            peek().token_value == "float" || peek().token_value == "char") {
            // For now, we assume it's a variable declaration. We'll add functions later.
            return parse_variable_declaration();
        }

        report_error("Unrecognized top-level statement. Expected a global variable or function.");
        throw runtime_error("Syntax Error");
    }

    // Rule: variable_declaration -> 'const'? type_specifier identifier_list ('=' expression)? ';'
    ParseNode* parse_variable_declaration() {
        int start_line = peek().line_number;
        ParseNode* decl_node = new ParseNode{"VariableDeclaration", "", start_line};

        if (peek().token_value == "const") {
            Token t = match("KEYWORD", "const");
            decl_node->children.push_back(new ParseNode{"Keyword", t.token_value, t.line_number});
        }

        Token type_token = match("KEYWORD");
        decl_node->children.push_back(new ParseNode{"TypeSpecifier", type_token.token_value, type_token.line_number});

        do {
            if (peek().token_value == ",") {
                match("SPECIAL CHARACTER", ",");
            }
            Token var_token = match("IDENTIFIER");
            decl_node->children.push_back(new ParseNode{"Identifier", var_token.token_value, var_token.line_number});
        } while (peek().token_value == ",");

        if (peek().token_value == "=") {
            match("OPERATOR", "=");
            // This part is a placeholder. We will implement parse_expression() later.
            Token value_token = match("NUMERIC CONSTANT");
            decl_node->children.push_back(new ParseNode{"Constant", value_token.token_value, value_token.line_number});
        }

        match("SPECIAL CHARACTER", ";");
        return decl_node;
    }
};

// --- FILE READING LOGIC ---

// This function reads the tokens.txt file and converts it into a vector<Token>.
vector<Token> load_tokens_from_file(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Fatal Error: Could not open token file '" << filename << "'" << endl;
        return {}; // Return an empty vector
    }

    vector<Token> loaded_tokens;
    string line;
    while (getline(file, line)) {
        if (line.length() < 5) continue; // Skip empty or invalid lines

        // Find the positions of the delimiters
        size_t first_comma = line.find(',');
        size_t last_comma = line.rfind(',');

        if (first_comma == string::npos || last_comma == string::npos || first_comma == last_comma) {
            cerr << "Warning: Malformed token line, skipping: " << line << endl;
            continue;
        }

        // Extract the parts using substr
        string token_class = line.substr(1, first_comma - 1);
        string token_value = line.substr(first_comma + 2, last_comma - (first_comma + 2));
        string line_str = line.substr(last_comma + 2, line.length() - last_comma - 3);

        Token t;
        t.token_class = token_class;
        t.token_value = token_value;
        try {
            t.line_number = stoi(line_str); // Convert string to integer
        } catch (...) {
            cerr << "Warning: Malformed line number, skipping: " << line << endl;
            continue;
        }
        loaded_tokens.push_back(t);
    }
    cout << "Token file loaded. " << loaded_tokens.size() << " tokens read." << endl;
    return loaded_tokens;
}


// --- MAIN FUNCTION ---

int main() {
    // The parser reads from the scanner's output file.
    const string token_file = "tokens.txt";

    // 1. Load tokens from the file.
    vector<Token> tokens = load_tokens_from_file(token_file);

    if (tokens.empty()) {
        cout << "No tokens to parse. Halting." << endl;
        return 1;
    }

    // 2. Create the parser and run it.
    cout << "---------------------------------" << endl;
    cout << "Starting Parser..." << endl;
    Parser parser(tokens);
    ParseNode* parse_tree = parser.parse();

    // 3. Check the result.
    cout << "---------------------------------" << endl;
    if (parse_tree != nullptr) {
        cout << "Program is syntactically valid." << endl;
        // We will add code to print the parse tree later.
        delete parse_tree; // Crucial: clean up the memory used by the tree.
    } else {
        cout << "Program has one or more syntax errors." << endl;
    }
    
    cout << "Press enter to end the program.";
    cin.get();
    return 0;
}