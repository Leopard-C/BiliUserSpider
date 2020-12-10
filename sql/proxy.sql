--
-- 数据表：proxy
-- 说明：代理IP池
--          host：代理IP主机（即IP地址）
--          port：代理IP端口
--          error_count: 使用此IP爬虫的总失败次数，失败次数过多该代理IP会被清除
--      没有失效时间（如ttl字段），是因为
--       1. 代理IP失效时间不固定（3-5分钟）
--       2. 即使失效时间固定也不应该根据时间清除代理IP
--         我认为这个失效时间可信度不高，而且如果代理根本就不能用，却还要在数据库中存在固定的时间，
--         后续的爬虫用这个IP只是在做无用功。
--
CREATE TABLE `proxy` (
  `host` varchar(16) NOT NULL,
  `port` int NOT NULL,
  `error_count` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`host`,`port`)
) ENGINE=MEMORY DEFAULT CHARSET=utf8;