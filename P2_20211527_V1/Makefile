# ssu_cleanupd 프로젝트를 위한 Makefile
# 이 파일은 ssu_cleanupd 데몬의 빌드 규칙과 설정을 정의합니다

# ======================
# 컴파일러 설정
# ======================

# 사용할 컴파일러 (GNU C 컴파일러)
CC = gcc

# 컴파일러 플래그:
# -Wall: 모든 경고 메시지 활성화
# -Wextra: 추가 경고 메시지 활성화  
# -g: 디버그 정보 포함
# -I.: 헤더 파일 검색 경로에 현재 디렉토리 추가
CFLAGS = -Wall -Wextra -g -I.

# 링커 플래그 (기본값은 비어 있음)
LDFLAGS = 

# 최종 생성할 실행 파일 이름
TARGET = ssu_cleanupd

# ======================
# 소스 파일 설정
# ======================

# 모든 C 소스 파일 목록
SRCS = ssu_cleanupd.c utils.c log.c file_ops.c daemon_process.c daemon_list.c config.c commands.c arrange.c

# 오브젝트 파일 목록 (.c -> .o 변환)
OBJS = $(SRCS:.c=.o)

# 모든 헤더 파일 목록
HEADERS = ssu_cleanupd.h utils.h log.h file_ops.h daemon_process.h daemon_list.h config.h commands.h

# ======================
# 가상 타겟 정의
# ======================
.PHONY: all clean debug release

# 기본 타겟 (make 명령어만 입력했을 때 실행)
all: $(TARGET)

# ======================
# 메인 타겟 빌드 규칙
# ======================

# 실행 파일 빌드 규칙
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# ======================
# 오브젝트 파일 빌드 패턴 규칙
# ======================

# .c 파일을 .o 파일로 컴파일하는 규칙
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# ======================
# 디버그 빌드 설정
# ======================

# 디버그 빌드: DEBUG 매크로 정의 및 디버그 정보 추가
debug: CFLAGS += -DDEBUG -g
debug: clean all

# ======================
# 릴리즈 빌드 설정  
# ======================

# 릴리즈 빌드: 최적화 활성화 및 디버그 매크로 제거
release: CFLAGS += -O2 -DNDEBUG
release: clean all

# ======================
# 정리 규칙
# ======================

# 생성된 모든 파일 삭제
clean:
	rm -f $(TARGET) $(OBJS)

# ======================
# 의존성 생성 설정 (선택적)
# ======================

# 의존성 파일을 저장할 디렉토리
DEPDIR := .d

# 의존성 디렉토리 생성
$(shell mkdir -p $(DEPDIR) >/dev/null)

# 의존성 생성 플래그
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

# 의존성 고려한 컴파일 명령 재정의
COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) -c

# 의존성 생성 규칙
%.o: %.c $(DEPDIR)/%.d
	$(COMPILE.c) $<
	@mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

# 빈 의존성 파일 생성 규칙
$(DEPDIR)/%.d: ;

# 생성된 의존성 파일 보존 설정
.PRECIOUS: $(DEPDIR)/%.d

# 모든 소스 파일의 의존성 파일 포함
-include $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS)))