# 컴파일러 설정
CC = gcc                      # 사용할 컴파일러
CFLAGS = -Wall -Wextra -Wformat-truncation -g  # 컴파일 경고 및 디버깅 옵션
TARGET = ssu_cleanup           # 생성할 실행 파일 이름
SRC = ssu_cleanup.c            # 소스 코드 파일

# 기본 빌드 명령어 (컴파일)
all: $(TARGET)

# 실행 파일 생성 (빌드)
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# 디버깅용 컴파일 (-DDEBUG 옵션 추가)
debug: $(SRC)
	$(CC) $(CFLAGS) -DDEBUG -o $(TARGET) $(SRC)

# 실행 명령 (빌드 후 실행)
run: $(TARGET)
	./$(TARGET)

# 불필요한 파일 제거 (클린업)
clean:
	rm -f $(TARGET) *.o

# clean, all, run 등이 실제 파일과 충돌하지 않도록 설정
.PHONY: all clean debug run
