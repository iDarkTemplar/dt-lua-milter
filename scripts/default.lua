--[[
/*
 * Copyright (C) 2016-2017 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *
 * This file is part of DT Lua Milter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
]]--

login_maps_filename = "/etc/postfix/login_maps"

-- configuration end

function parse_sender(sender_string)
	local parsed_sender, parsed_subaddress, parsed_domain
	local remaining_string = sender_string

	-- first look for '<' and '>' symbols, if both are present and
	local i, _ = string.find(remaining_string, "<", 1, true)
	if i ~= nil then
		local j, _ = string.find(remaining_string, ">", i + 1, true)

		if j ~= nil then
			local prefix = string.sub(remaining_string, 1, i - 1);
			local suffix = string.sub(remaining_string, j + 1);
			remaining_string = string.sub(remaining_string, i + 1, j - 1)

			local k, _ = string.find(prefix, "[<>]")
			local l, _ = string.find(suffix, "[<>]")
			local m, _ = string.find(remaining_string, "[<>]")
			if (k ~= nil) or (l ~= nil) or (m ~= nil) then
				return false -- it is not a valid address in format "User Name <user@domain>"
			end
		else
			return false -- it is not a valid address in format "User Name <user@domain>"
		end
	end

	-- parse sender address
	i, _ = string.find(remaining_string, "[+@]");

	if i ~= nil then
		parsed_sender = string.sub(remaining_string, 1, i - 1)

		local found_sign = string.sub(remaining_string, i, i)

		remaining_string = string.sub(remaining_string, i + 1)

		if found_sign == "@" then
			-- only domain remains
			i, _ = string.find(remaining_string, "[+@]");

			if i ~= nil then
				return false -- domain shouldn't contain '+' or '@' symbols
			end

			parsed_domain = remaining_string

			if string.len(parsed_domain) == 0 then
				return false
			end
		else
			-- subaddress remains, maybe with domain
			i, _ = string.find(remaining_string, "[+@]")

			if i ~= nil then
				found_sign = string.sub(remaining_string, i, i)

				if found_sign ~= "@" then
					return false -- it shouldn't contain multiple '+' symbols
				end

				-- subaddress + domain
				parsed_subaddress = string.sub(remaining_string, 1, i - 1)

				remaining_string = string.sub(remaining_string, i + 1)

				-- only domain remains
				i, k = string.find(remaining_string, "[+@]");

				if i ~= nil then
					return false -- domain shouldn't contain '+' or '@' symbols
				end

				parsed_domain = remaining_string

				if string.len(parsed_domain) == 0 then
					return false
				end
			else
				-- no domain specified, only subaddress
				parsed_subaddress = remaining_string
			end

			if string.len(parsed_subaddress) == 0 then
				return false
			end
		end
	else
		-- only sender address present
		parsed_sender = remaining_string
	end

	if string.len(parsed_sender) == 0 then
		return false
	end

	return true, parsed_sender, parsed_subaddress, parsed_domain
end

function read_login_maps(filename)
	local result_table = {}
	local file = io.open(filename, "r");

	if file ~= nil then
		local line
		for line in file:lines() do
			local parsed_login, parsed_map = string.match(line, "^[%s]*([^%s,]+)[%s,]+(.+)$")

			if (parsed_login ~= nil) and (parsed_map ~= nil) then
				local parsed_login_lowercase = string.lower(parsed_login)

				while string.len(parsed_map) > 0 do
					local value, remaining_value = string.match(parsed_map, "^[%s,]*([^%s,]+)[%s,]+(.+)$")

					if value ~= nil then
						-- found match, we have current address and remainder

						-- only append to list. This allows to list valid email addresses for one user on multiple lines.
						if result_table[parsed_login_lowercase] == nil then
							result_table[parsed_login_lowercase] = {}
						end

						result_table[parsed_login_lowercase][string.lower(value)] = true
						parsed_map = remaining_value
					else
						-- no match, we may have one final value in the parsed_map

						value = string.match(parsed_map, "^[%s,]*([^%s,]+)[%s,]*$")
						if value ~= nil then
							-- we have one value there

							-- only append to list. This allows to list valid email addresses for one user on multiple lines.
							if result_table[parsed_login_lowercase] == nil then
								result_table[parsed_login_lowercase] = {}
							end

							result_table[parsed_login_lowercase][string.lower(value)] = true
						end

						parsed_map = ""
					end
				end
			end
		end

		file:close()
	end

	return result_table
end

login_maps = read_login_maps(login_maps_filename)

-- All callbacks are below

function mlfi_connect(hostname)
    return SMFIS_CONTINUE
end

function mlfi_helo(helohost)
    return SMFIS_CONTINUE
end

function mlfi_envfrom(...)
    headers = {} -- clean headers
    sender = smfi_getsymval("{mail_addr}")
    return SMFIS_CONTINUE
end

function mlfi_envrcpt(...)
    return SMFIS_CONTINUE
end

function mlfi_header(header, header_value)
	if string.lower(header) == "from" then
		local envelope_sender = string.lower(sender)
		local header_sender = string.lower(header_value)
		local parsed_ok, parsed_user, parsed_subaddress, parsed_domain = parse_sender(header_sender)

		if not parsed_ok then
			smfi_setreply("550", nil, "failed to parse \"from\" header: " .. header_value)
			return SMFIS_REJECT
		end

		local final_user = parsed_user

		if parsed_domain ~= nil then
			final_user = final_user .. "@" .. parsed_domain
		end

		local match_is_good = false

		if login_maps[envelope_sender] then
			-- there is user in the login maps, check map

			if login_maps[envelope_sender][final_user] then
				match_is_good = true
			end
		else
			-- no such user in login maps, fallback to check that envelope and header match

			if final_user == envelope_sender then
				match_is_good = true
			end
		end

		if not match_is_good then
			smfi_setreply("550", nil, "your user is not allowed to change \"from\" header to: " .. header_value)
			return SMFIS_REJECT
		end
    end

    return SMFIS_CONTINUE
end

function mlfi_eoh()
    return SMFIS_CONTINUE
end

function mlfi_body(body, body_len)
    return SMFIS_CONTINUE
end

function mlfi_eom()
    return SMFIS_CONTINUE
end

function mlfi_abort()
    return SMFIS_CONTINUE
end

function mlfi_close()
    return SMFIS_CONTINUE
end

function mlfi_unknown(cmd)
    return SMFIS_CONTINUE
end

function mlfi_data()
    return SMFIS_CONTINUE
end
