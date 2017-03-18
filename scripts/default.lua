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
		for line in file:lines() do
			-- example: mail@localhost      mail@localhost,root@localhost,webmaster@localhost,postmaster@localhost
			-- TODO: parse file line
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

		-- TODO: check users map
		--[[
		local file = io.open("/tmp/dt-lua.log", "a+")
		file:write(string.format("envelope: %s, header: %s\nenvelope: %s, sender: %s\n", tostring(sender), tostring(header_value), tostring(envelope_sender), tostring(header_sender)))
		file:write(string.format("parsed: %s, sender: %s, subaddress: %s, domain: %s\n", tostring(parsed_ok), tostring(parsed_user), tostring(parsed_subaddress), tostring(parsed_domain)))
		file:write(string.format("final_user: %s\n", final_user))
		file:close()
		]]--
		
		if final_user ~= envelope_sender then
			smfi_setreply("550", nil, "your user is not allowed to change \"from\" header to: " .. header_value)
			--smfi_setreply("550", nil, "your user is not allowed to change \"from\" header to: " .. header_value .. ", envelope sender: " .. sender .. ", final user: " .. final_user)
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
