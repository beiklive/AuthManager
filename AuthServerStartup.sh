#!/bin/bash

# 记录脚本的原始目录
script_dir="$(pwd)"

# AuthManager 进程名字
auth_manager_process_name="AuthManager"

# 启动第一个进程（监控进程）
start_monitor_process() {
    while true; do
        # 遍历目录下的所有文件
        for file in "$monitor_dir"/*; do
            if [ -f "$file" ]; then
                # 监控文件的修改状态
                if [ "$file" -nt "/tmp/last_modified" ]; then
                    echo "File $file has been modified. Restarting AuthManager..."
                    # 杀死 AuthManager 进程
                    stop_auth_manager
                    # 启动 AuthManager 进程
                    cd "$script_dir/bin" || exit 1
                    nohup ./AuthManager --log-level DEBUG > /dev/null 2>&1 &
                    # 更新最后修改时间记录
                    touch /tmp/last_modified
                    cd "$script_dir" || exit 1
                    break
                fi
            fi
        done
        sleep 1  # 等待一秒钟后再次检查
    done
}

# 启动 AuthManager 进程
start_auth_manager() {
    if pgrep -x "$auth_manager_process_name" > /dev/null; then
        echo "AuthManager is already running."
    else
        cd "$script_dir/bin" || exit 1
        nohup ./AuthManager --log-level DEBUG > /dev/null 2>&1 &
        echo "AuthManager has been started."
        cd "$script_dir" || exit 1
    fi
}

# 停止 AuthManager 进程
stop_auth_manager() {
    if pgrep -x "$auth_manager_process_name" > /dev/null; then
        echo "Stopping AuthManager..."
        pkill -x "$auth_manager_process_name"
    else
        echo "AuthManager is not running."
    fi

    # 检查是否有启动脚本的后台进程，并杀死它们
    if [ -f "AuthServerStartup.pid" ]; then
        start_pid=$(cat "AuthServerStartup.pid")
        if [ -n "$start_pid" ] && ps -p "$start_pid" > /dev/null; then
            echo "Stopping AuthServerStartup process..."
            kill -15 "$start_pid"
        fi
        rm "AuthServerStartup.pid"
    fi
}

# 显示 AuthManager 进程状态
show_auth_manager_status() {
    if pgrep -x "$auth_manager_process_name" > /dev/null; then
        auth_manager_pid=$(pgrep -x "$auth_manager_process_name")
        echo -e "AuthManager is running with PID $auth_manager_pid."
        echo -e "Process details:"
        ps -p "$auth_manager_pid" -o pid,ppid,pgid,%cpu,%mem,state,comm,cwd --sort=-%cpu | tail -n +2 | awk 'BEGIN {printf "%-6s %-6s %-6s %-6s %-6s %-6s %-20s %-50s\n", "PID", "PPID", "PGID", "%CPU", "%MEM", "STATE", "COMMAND", "CWD"} {printf "%-6s %-6s %-6s %-6s %-6s %-6s %-20s %-50s\n", $1, $2, $3, $4, $5, $6, $7, $8}'
    else
        echo "AuthManager is not running."
    fi
}

clean_auth_cache(){
    cd "$script_dir/bin/log" || exit 1
    rm *.log
}


# 处理命令行参数
case "$1" in
    start)
        start_monitor_process > /dev/null 2>&1 &
        echo "$!" > "AuthServerStartup.pid"  # 将start命令的进程ID写入AuthServerStartup.pid文件
        start_auth_manager
        show_auth_manager_status
        ;;
    stop)
        stop_auth_manager
        ;;
    status)
        show_auth_manager_status
        ;;
    clean)
        clean_auth_cache
        ;;
    *)
        echo "Usage: $0 {start|stop|status}"
        exit 1
        ;;
esac
