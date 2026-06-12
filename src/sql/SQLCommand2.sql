-- ============================================================
-- ChatServer 数据库初始化脚本
-- 用途：创建用户、数据库及所有表结构
-- 执行方式：mysql -u root -p < init.sql
-- ============================================================

-- 1. 创建数据库用户（如已存在则先删除）
DROP USER IF EXISTS 'LiteChat_user'@'localhost';
CREATE USER 'LiteChat_user'@'localhost' IDENTIFIED BY 'LiteChat_password';
GRANT ALL PRIVILEGES ON LiteChat_db.* TO 'LiteChat_user'@'localhost';
FLUSH PRIVILEGES;

-- 2. 创建数据库
DROP DATABASE IF EXISTS LiteChat_db;
CREATE DATABASE LiteChat_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

USE LiteChat_db;

-- 3. 用户表（User）
--  来源：LLD 2.1.1
CREATE TABLE `user` (
    `user_id`     INT NOT NULL AUTO_INCREMENT COMMENT '用户ID，主键',
    `username`    VARCHAR(20) NOT NULL COMMENT '用户名，唯一',
    `password`    VARCHAR(32) NOT NULL COMMENT '密码，MD5加密(32位十六进制)',
    `nickname`    VARCHAR(20) DEFAULT NULL COMMENT '昵称，默认等于username',
    `status`      TINYINT NOT NULL DEFAULT 0 COMMENT '状态：0-离线，1-在线',
    `create_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    `update_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (`user_id`),
    UNIQUE KEY `uk_username` (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='用户表';

-- 4. 好友关系表（Friend）
--  来源：LLD 2.1.2
--  说明：好友关系为双向对等，添加好友时插入两条记录（user_id=A,friend_id=B 和 user_id=B,friend_id=A）
--        ON DELETE CASCADE 确保删除用户时级联删除好友关系
CREATE TABLE `friend` (
    `id`          INT NOT NULL AUTO_INCREMENT COMMENT '关系ID，主键',
    `user_id`     INT NOT NULL COMMENT '用户ID，外键',
    `friend_id`   INT NOT NULL COMMENT '好友ID，外键',
    `create_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_user_friend` (`user_id`, `friend_id`),
    KEY `idx_friend_id` (`friend_id`),
    CONSTRAINT `fk_friend_user1` FOREIGN KEY (`user_id`) REFERENCES `user` (`user_id`) ON DELETE CASCADE,
    CONSTRAINT `fk_friend_user2` FOREIGN KEY (`friend_id`) REFERENCES `user` (`user_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='好友关系表';

-- 5. 消息表（Message）
--  来源：LLD 2.1.3
--  说明：联合索引(idx_sender_time, idx_receiver_time)用于加速历史消息按时间排序查询
--        ON DELETE CASCADE 确保删除用户时级联删除消息记录
CREATE TABLE `message` (
    `msg_id`      INT NOT NULL AUTO_INCREMENT COMMENT '消息ID，主键',
    `sender_id`   INT NOT NULL COMMENT '发送者ID，外键',
    `receiver_id` INT NOT NULL COMMENT '接收者ID，外键',
    `content`     TEXT NOT NULL COMMENT '消息内容，UTF-8编码，最大64KB',
    `send_time`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '发送时间',
    `is_read`     TINYINT NOT NULL DEFAULT 0 COMMENT '是否已读：0-未读，1-已读',
    PRIMARY KEY (`msg_id`),
    KEY `idx_sender_time` (`sender_id`, `send_time`),
    KEY `idx_receiver_time` (`receiver_id`, `send_time`),
    CONSTRAINT `fk_msg_sender` FOREIGN KEY (`sender_id`) REFERENCES `user` (`user_id`) ON DELETE CASCADE,
    CONSTRAINT `fk_msg_receiver` FOREIGN KEY (`receiver_id`) REFERENCES `user` (`user_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='消息表';
