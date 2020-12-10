--
-- 数据表：mid_0_200
-- 说明：0_200表示用户ID为0-200w的用户信息（用户存在且信息非空）
--       包括0，不包括200w
--
CREATE TABLE `mid_0_200` (
  `mid` int unsigned NOT NULL,
  `name` varchar(80) DEFAULT NULL,
  `sex` tinyint DEFAULT NULL,
  `face` varchar(100) DEFAULT NULL,
  `sign` varchar(200) DEFAULT NULL,
  `level` tinyint DEFAULT NULL,
  `attention` int DEFAULT NULL,
  `fans` int DEFAULT NULL,
  `vip_status` tinyint DEFAULT NULL,
  `vip_type` tinyint DEFAULT NULL,
  `vip_label` varchar(20) DEFAULT NULL,
  `official_type` tinyint DEFAULT NULL,
  `official_desc` varchar(100) DEFAULT NULL,
  `official_role` tinyint DEFAULT NULL,
  `official_title` varchar(100) DEFAULT NULL,
  `lid` int unsigned DEFAULT NULL,
  `is_deleted` tinyint DEFAULT NULL,
  PRIMARY KEY (`mid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;