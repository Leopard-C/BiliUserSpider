--
-- 数据表：mid_none_0_200
-- 说明：0_200表示用户ID为0-200w的用户ID（用户不存在 或者 用户信息为空）
--       包括0，不包括200w
--       只存储用户ID（即mid，或者称之为uid）
--       该表和mid_0_200的数据（mid字段）互补，存在的意义是检校是否有遗漏（两个表数据综合200w，且没有重复数据）
--
CREATE TABLE `mid_none_0_200` (
  `mid` int unsigned NOT NULL,
  PRIMARY KEY (`mid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
