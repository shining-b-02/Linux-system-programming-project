# Makefile for ssu_ext2

# 컴파일러 설정
CC      = gcc

# 컴파일 옵션 설정
# -std=c11                 : C11 표준 사용
# -D_POSIX_C_SOURCE=200809L: getline 등 POSIX 기능 활성화
# -Wall -Wextra            : 모든 경고 활성화
# -g                       : 디버그 심볼 포함
CFLAGS  = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -g

# 최종 생성할 실행 파일 이름
TARGET  = ssu_ext2

# 소스 파일 목록
SRC     = ssu_ext2.c

# 오브젝트 파일 목록 (.c → .o)
OBJ     = $(SRC:.c=.o)

# 가상(target) 선언: clean, all 은 파일명이 아닌 명령어
.PHONY: all clean

# 기본 타겟: make 또는 make all 시 실행
all: $(TARGET)

# 실행 파일 생성 규칙
# $@ = TARGET, $^ = 의존 대상(OBJ)
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# .c 파일로부터 .o 파일을 생성하는 패턴 규칙
# $< = 첫 번째 의존 대상(%.c), $@ = 출력 파일(%.o)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# clean 타겟: 빌드 결과물 삭제
clean:
	rm -f $(OBJ) $(TARGET)
