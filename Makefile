CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall
TARGET   = gioco
SRC      = src/menu.cpp

.PHONY: all clean run backend install

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

backend:
	python3 backend/app.py &

install: $(TARGET)
	@echo "Installazione completata. Avvia con: make run"

clean:
	rm -f $(TARGET)
