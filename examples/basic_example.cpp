#include "memorydb/memorydb.h"
#include <iostream>
#include <string>

using namespace memorydb;

void printSeparator() {
    std::cout << std::string(50, '-') << std::endl;
}

int main() {
    std::cout << "Memory Database Basic Example" << std::endl;
    printSeparator();

    // Create database
    auto db = createDatabase();

    // Basic operations
    std::cout << "\n1. Basic Put/Get Operations:" << std::endl;
    db->put("name", std::string("MemoryDB"));
    db->put("version", 1);
    db->put("performance", 99.9);

    Value value;
    db->get("name", value);
    std::cout << "   name: " << std::get<std::string>(value) << std::endl;

    db->get("version", value);
    std::cout << "   version: " << std::get<int64_t>(value) << std::endl;

    db->get("performance", value);
    std::cout << "   performance: " << std::get<double>(value) << std::endl;

    // Update operation
    std::cout << "\n2. Update Operation:" << std::endl;
    std::cout << "   Old version: ";
    db->get("version", value);
    std::cout << std::get<int64_t>(value) << std::endl;

    db->put("version", 2);
    std::cout << "   New version: ";
    db->get("version", value);
    std::cout << std::get<int64_t>(value) << std::endl;

    // Check existence
    std::cout << "\n3. Existence Check:" << std::endl;
    std::cout << "   'name' exists: " << std::boolalpha << db->exists("name") << std::endl;
    std::cout << "   'unknown' exists: " << db->exists("unknown") << std::endl;

    // Size
    std::cout << "\n4. Database Size:" << std::endl;
    std::cout << "   Total keys: " << db->size() << std::endl;

    // Delete operation
    std::cout << "\n5. Delete Operation:" << std::endl;
    std::cout << "   Removing 'performance'..." << std::endl;
    db->remove("performance");
    std::cout << "   Total keys after removal: " << db->size() << std::endl;

    // Transaction example
    std::cout << "\n6. Transaction Example:" << std::endl;
    db->put("balance", 1000);
    std::cout << "   Initial balance: ";
    db->get("balance", value);
    std::cout << std::get<int64_t>(value) << std::endl;

    std::cout << "\n   Starting transaction..." << std::endl;
    db->beginTransaction();
    db->put("balance", 500);
    db->put("withdrawn", 500);
    std::cout << "   Balance in transaction: ";
    db->get("balance", value);
    std::cout << std::get<int64_t>(value) << std::endl;

    std::cout << "   Committing transaction..." << std::endl;
    db->commitTransaction();
    std::cout << "   Balance after commit: ";
    db->get("balance", value);
    std::cout << std::get<int64_t>(value) << std::endl;

    // Transaction rollback
    std::cout << "\n7. Transaction Rollback Example:" << std::endl;
    db->beginTransaction();
    db->put("balance", 0);
    std::cout << "   Set balance to 0 in transaction" << std::endl;
    std::cout << "   Rolling back..." << std::endl;
    db->rollbackTransaction();
    std::cout << "   Balance after rollback: ";
    db->get("balance", value);
    std::cout << std::get<int64_t>(value) << std::endl;

    printSeparator();
    std::cout << "\nExample completed successfully!" << std::endl;

    return 0;
}
